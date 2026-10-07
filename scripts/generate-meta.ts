// Node >= 24. Usage: node scripts/generate-meta.ts [clang-wasm asset directory]
import { mkdir, readFile, readdir, writeFile } from 'node:fs/promises';
import path from 'node:path';
import process from 'node:process';
import { pathToFileURL } from 'node:url';
import { gunzipSync } from 'node:zlib';
import { Worker, isMainThread, parentPort, workerData } from 'node:worker_threads';

interface ClangFileSystem {
  mkdirTree(path: string): void;
  writeFile(path: string, data: string | Uint8Array): void;
  readFile(path: string, options: { encoding: 'utf8' }): string;
  getStream(fd: number): { fd: number };
  open(path: string, mode: string): { fd: number };
  close(stream: { fd: number }): void;
  analyzePath(path: string): { exists: boolean };
}

interface ClangOptions {
  noInitialRun: boolean;
  thisProgram: string;
  printErr(text: string): void;
  instantiateWasm(
    imports: WebAssembly.Imports,
    receive: (instance: WebAssembly.Instance, module: WebAssembly.Module) => void,
  ): WebAssembly.Exports;
}

interface ClangModule {
  FS: ClangFileSystem;
  callMain(args: string[]): number;
}

interface AstType {
  qualType: string;
  desugaredQualType?: string;
}

interface AstNode {
  kind: string;
  name?: string;
  text?: string;
  loc?: { file?: string; offset: number; tokLen: number };
  inner?: AstNode[];
  type?: AstType;
  defaultArg?: { type?: AstType };
  bases?: { type: AstType }[];
  isImplicit?: boolean;
  completeDefinition?: boolean;
}

export interface MetadataEntry {
  id: string;
  namespace: string;
  name: string;
  description: string;
  icon: string;
}

export interface ParameterMetadata extends MetadataEntry {
  control: Record<string, unknown>;
}

export interface PortMetadata extends MetadataEntry {
  vectorized?: true;
}

export interface BlockMetadata extends MetadataEntry {
  inputs: PortMetadata[];
  outputs: PortMetadata[];
  parameters: ParameterMetadata[];
}

export interface Metadata {
  namespaces: Omit<MetadataEntry, 'namespace'>[];
  blocks: BlockMetadata[];
}

interface Declaration {
  node: AstNode;
  scope: string[];
  template?: AstNode;
  meta: MetadataEntry;
}

const root = path.resolve(import.meta.dirname, '..');

// This release supports web/worker environments only. Keep its browser shim in
// a worker so the main Node environment remains intact; use the original JS glue.
async function compile(assets: string): Promise<Metadata> {
  const wasm = await readFile(path.join(assets, 'clang.wasm'));
  const archive = await readFile(path.join(assets, 'sysroot.tgz'));
  const { default: createClang }: { default: (options: ClangOptions) => Promise<ClangModule> } =
    await import(pathToFileURL(path.join(assets, 'clang.js')).href);
  const browserGlobals = globalThis as unknown as { process: NodeJS.Process | undefined; window: object };
  browserGlobals.process = undefined;
  browserGlobals.window = {};
  const clang = await createClang({
    noInitialRun: true,
    thisProgram: '/clang',
    printErr: text => console.error(text),
    instantiateWasm(imports, receive) {
      const module = new WebAssembly.Module(wasm);
      const instance = new WebAssembly.Instance(module, imports);
      receive(instance, module);
      return instance.exports;
    },
  });
  const { FS } = clang;
  installSysroot(FS, archive);

  const sources = new Map<string, Buffer>();
  const sourceRoot = path.join(root, 'src');
  const entries = await readdir(sourceRoot, { recursive: true, withFileTypes: true });
  for (const entry of entries.values().filter(entry => entry.isFile() && entry.name.endsWith('.hpp'))) {
    const source = path.join(entry.parentPath, entry.name);
    const relative = path.relative(sourceRoot, source).split(path.sep).join('/');
    const filename = `/project/src/${relative}`;
    const content = await readFile(source);
    FS.mkdirTree(path.posix.dirname(filename));
    FS.writeFile(filename, content);
    sources.set(filename, content);
  }
  if (!sources.size) throw new Error('No .hpp files found under src/');
  const headers = [...sources.keys()].sort();
  FS.writeFile('/project/meta.cpp', headers.map(file => `#include "${file}"`).join('\n'));

  // Write stdout directly into MEMFS: print() loses the final JSON brace because
  // Clang does not append a newline and Emscripten's TTY buffers incomplete lines.
  FS.close(FS.getStream(1));
  const stdout = FS.open('/ast.json', 'w');
  if (stdout.fd !== 1) throw new Error('Cannot redirect clang stdout');
  const status = clang.callMain([
    '-cc1', '-triple', 'wasm32-unknown-emscripten', '-std=c++23', '-x', 'c++',
    '-isysroot', '/sysroot', '-resource-dir', '/sysroot/lib/clang/23',
    '-isystem', '/sysroot/include/c++/v1',
    '-isystem', '/sysroot/lib/clang/23/include',
    '-isystem', '/sysroot/include/compat', // Emscripten's xlocale.h and C header shims.
    '-isystem', '/sysroot/include',
    '-I', '/project/src', '-fparse-all-comments', '-ast-dump=json', '/project/meta.cpp',
  ]);
  FS.close(stdout);
  if (status !== 0) throw new Error(`clang failed with exit code ${status}`);
  return extractMetadata(JSON.parse(FS.readFile('/ast.json', { encoding: 'utf8' })), sources);
}

// Extract the release's ustar archive straight into clang's in-memory filesystem.
// No host tar executable, host sysroot, or npm packages are required.
function installSysroot(FS: ClangFileSystem, archive: Buffer): void {
  const tar = gunzipSync(archive);
  const string = (bytes: Buffer) => bytes.toString('utf8').split('\0', 1)[0];
  for (let offset = 0; offset + 512 <= tar.length;) {
    const header = tar.subarray(offset, offset + 512);
    if (header.every(byte => byte === 0)) break;
    const size = Number.parseInt(string(header.subarray(124, 136)).trim(), 8);
    const checksum = Number.parseInt(string(header.subarray(148, 156)).trim(), 8);
    const actual = header.reduce((sum, byte, i) => sum + (i >= 148 && i < 156 ? 32 : byte), 0);
    if (checksum !== actual || !Number.isSafeInteger(size) || size < 0 || offset + 512 + size > tar.length) {
      throw new Error('Invalid or truncated sysroot tar archive');
    }
    const prefix = string(header.subarray(345, 500));
    const name = `${prefix ? `${prefix}/` : ''}${string(header.subarray(0, 100))}`;
    if (!name.startsWith('sysroot/') || name.split('/').includes('..')) {
      throw new Error(`Unexpected sysroot archive path: ${name}`);
    }
    const filename = `/${name}`;
    const kind = header[156];
    if (kind === 53) {
      FS.mkdirTree(filename);
    } else if (kind === 48 || kind === 0) {
      FS.mkdirTree(path.posix.dirname(filename));
      FS.writeFile(filename, tar.subarray(offset + 512, offset + 512 + size));
    } else {
      throw new Error(`Unsupported sysroot tar entry: ${name} (type ${kind})`);
    }
    offset += 512 + Math.ceil(size / 512) * 512;
  }
  if (!FS.analyzePath('/sysroot/include/c++/v1/type_traits').exists) {
    throw new Error('sysroot.tgz does not contain the expected C++ headers');
  }
}

function extractMetadata(ast: AstNode, sources: Map<string, Buffer>): Metadata {
  // Clang omits filenames when they match the preceding emitted location.
  // Restore them in JSON property order, excluding include-stack locations.
  let lastFile: string | undefined;
  function restoreFiles(value: unknown): void {
    if (!value || typeof value !== 'object') return;
    const record = value as Record<string, unknown>;
    if ('offset' in record && 'col' in record) {
      if (typeof record.file === 'string') lastFile = record.file;
      else record.file = lastFile;
    }
    for (const [key, child] of Object.entries(value)) {
      if (key !== 'includedFrom') restoreFiles(child);
    }
  }
  restoreFiles(ast);

  const children = (node?: AstNode): AstNode[] => node?.inner ?? [];
  const fullComment = (node: AstNode) => children(node).find(child => child.kind === 'FullComment');
  const commentText = (node: AstNode): string => [node.text ?? '', ...children(node).map(commentText)]
    .join(' ').replace(/\s+/g, ' ').trim();
  function metadata(node: AstNode, scope: string[], comment = fullComment(node)): MetadataEntry {
    const parts = Object.groupBy(children(comment), part => part.kind);
    const paragraphs = (parts.ParagraphComment ?? []).map(commentText).filter(Boolean);
    const descriptions = (parts.BlockCommandComment ?? []).filter(part => {
      switch (part.name) {
        case 'brief':
        case 'details':
          return true;
        default:
          return false;
      }
    }).map(commentText);
    // JSON omits the command name on VerbatimLineComment, so consult its source
    // location to distinguish @image from other verbatim commands such as @file.
    const image = parts.VerbatimLineComment?.find(part => {
      const loc = part.loc;
      return !!loc?.file && sources.get(loc.file)?.subarray(loc.offset, loc.offset + loc.tokLen).toString() === 'image';
    });
    const icon = image?.text?.trim().replace(/^(?:html|latex|docbook|rtf|xml)\s+/, '').split(/\s+/, 1)[0] ?? '';
    return {
      id: node.name ?? '',
      namespace: scope.join('::'),
      name: paragraphs[0] ?? node.name ?? '',
      description: [...descriptions, ...paragraphs.slice(1)].filter(Boolean).join('\n\n'),
      icon,
    };
  }

  const declarations = new Map<string, Declaration>();
  const specializations = new Map<string, AstNode>();
  const namespaces = new Map<string, Omit<MetadataEntry, 'namespace'>>();
  const qualified = (scope: string[], name: string) => [...scope, name].join('::');
  function visit(node: AstNode, scope: string[] = [], template?: AstNode): void {
    if (node.kind === 'ClassTemplateSpecializationDecl' && node.completeDefinition && node.name) {
      const arguments_ = children(node).filter(child => child.kind === 'TemplateArgument');
      if (arguments_.every(argument => argument.type)) {
        const types = arguments_.map(argument => argument.type!.desugaredQualType ?? argument.type!.qualType);
        specializations.set(`${qualified(scope, node.name)}<${types.join(', ')}>`, node);
      }
      return;
    }
    if (node.isImplicit || node.kind.includes('Specialization')) return;
    const local = !!node.loc?.file && sources.has(node.loc.file);
    switch (node.kind) {
      case 'NamespaceDecl':
        if (!node.name) return;
        if (local) {
          const key = qualified(scope, node.name);
          if (fullComment(node) || !namespaces.has(key)) {
            const { name, description, icon } = metadata(node, scope);
            namespaces.set(key, { id: key, name, description, icon });
          }
        }
        for (const child of children(node)) visit(child, [...scope, node.name]);
        break;
      case 'ClassTemplateDecl':
      case 'TypeAliasTemplateDecl':
        for (const child of children(node)) {
          switch (child.kind) {
            case 'CXXRecordDecl':
            case 'TypeAliasDecl':
            case 'ClassTemplateSpecializationDecl':
              visit(child, scope, node);
              break;
          }
        }
        break;
      case 'CXXRecordDecl':
      case 'RecordDecl':
      case 'EnumDecl':
      case 'TypeAliasDecl':
      case 'TypedefDecl':
        if (!local || !node.name) return;
        if (node.kind.endsWith('RecordDecl') && !node.completeDefinition) return;
        declarations.set(qualified(scope, node.name), {
          node, scope, template, meta: metadata(node, scope, fullComment(template ?? node) ?? fullComment(node)),
        });
        if (node.kind.endsWith('RecordDecl')) {
          for (const child of children(node)) visit(child, [...scope, node.name]);
        }
        break;
      case 'TranslationUnitDecl':
      case 'LinkageSpecDecl':
      case 'ExportDecl':
        for (const child of children(node)) visit(child, scope);
        break;
    }
  }
  visit(ast);

  function resolve(type: string, scope: string[]): Declaration | undefined {
    const name = type.replace(/<.*>$/, '').replace(/^::/, '');
    for (let length = type.startsWith('::') ? 0 : scope.length; length >= 0; length--) {
      const found = declarations.get(qualified(scope.slice(0, length), name));
      if (found) return found;
    }
  }
  function isBlock(declaration: Declaration, seen = new Set<Declaration>()): boolean {
    if (seen.has(declaration)) return false;
    seen.add(declaration);
    if (declaration.node.name === 'Block' && !declaration.scope.length) return true;
    switch (declaration.node.kind) {
      case 'TypeAliasDecl':
      case 'TypedefDecl': {
        const type = declaration.node.type;
        if (!type) throw new Error(`Missing type for alias ${declaration.node.name}`);
        const target = resolve(type.desugaredQualType ?? type.qualType, declaration.scope);
        return !!target && isBlock(target, seen);
      }
    }
    return (declaration.node.bases ?? []).some(base => {
      const parent = resolve(base.type.desugaredQualType ?? base.type.qualType, declaration.scope);
      return parent && isBlock(parent, seen);
    });
  }
  function firstTypeArgument(spelling: string): string | undefined {
    const start = spelling.indexOf('<') + 1;
    if (!start) return undefined;
    let depth = 0;
    for (let i = start; i < spelling.length; i++) {
      const char = spelling[i];
      if (depth === 0 && (char === ',' || char === '>')) return spelling.slice(start, i).trim();
      if (char === '<' || char === '(') depth++;
      else if (char === '>' || char === ')') depth--;
    }
  }

  function isVectorized(type: AstType | undefined, scope: string[], seen = new Set<Declaration>()): boolean {
    if (!type) return false;
    for (const spelling of [type.qualType, type.desugaredQualType].filter(value => value !== undefined)) {
      const name = spelling.split('<', 1)[0].replace(/^(?:(?:const|volatile)\s+)+/, '').trim();
      if (/(?:^|::)Vectorized(?:Input|Output)$/.test(name)) return true;
      const declaration = resolve(name, scope);
      if (declaration && !seen.has(declaration) &&
          (declaration.node.kind === 'TypeAliasDecl' || declaration.node.kind === 'TypedefDecl')) {
        seen.add(declaration);
        if (isVectorized(declaration.node.type, declaration.scope, seen)) return true;
      }
      if (/^std::(?:\w+::)*(?:vector|function)$/.test(name)) {
        const argument = firstTypeArgument(spelling);
        if (argument && (name.endsWith('::vector') && /\*\s*(?:(?:const|volatile)\s*)*$/.test(argument) ||
            isVectorized({ qualType: argument }, scope, seen))) return true;
      }
    }
    return false;
  }

  function ports(type: AstType, scope: string[]): PortMetadata[] {
    const spelling = type.desugaredQualType ?? type.qualType;
    if (spelling === 'void') return [];
    const declaration = resolve(spelling, scope);
    if (!declaration) throw new Error(`Cannot resolve port struct ${spelling} in ${scope.join('::')}`);
    switch (declaration.node.kind) {
      case 'TypeAliasDecl':
      case 'TypedefDecl':
        if (!declaration.node.type) throw new Error(`Missing type for alias ${spelling}`);
        return ports(declaration.node.type, declaration.scope);
    }
    if (!declaration.node.completeDefinition) throw new Error(`Incomplete port struct: ${spelling}`);
    const inherited = (declaration.node.bases ?? []).flatMap(base => ports(base.type, declaration.scope));
    const fields = children(declaration.node).filter(child => child.kind === 'FieldDecl' && !child.isImplicit);
    return [...inherited, ...fields.map(field => {
      const port: PortMetadata = metadata(field, declaration.scope);
      if (isVectorized(field.type, [...declaration.scope, declaration.node.name ?? ''])) port.vectorized = true;
      return port;
    })];
  }

  function findConstructor(declaration: Declaration, seen = new Set<Declaration>()): { ctor: AstNode; decl: Declaration } | undefined {
    if (seen.has(declaration)) return undefined;
    seen.add(declaration);
    if (declaration.node.kind === 'TypeAliasDecl' || declaration.node.kind === 'TypedefDecl') {
      const type = declaration.node.type;
      if (!type) return undefined;
      const target = resolve(type.desugaredQualType ?? type.qualType, declaration.scope);
      return target ? findConstructor(target, seen) : undefined;
    }
    const ctors = children(declaration.node).filter(child => child.kind === 'CXXConstructorDecl' && !child.isImplicit);
    const ctorWithParams = ctors.find(ctor =>
      children(ctor).some(child => child.kind === 'ParmVarDecl' && child.name !== 'blockId')
    );
    if (ctorWithParams) return { ctor: ctorWithParams, decl: declaration };
    const ctorWithComments = ctors.find(ctor => fullComment(ctor));
    if (ctorWithComments) return { ctor: ctorWithComments, decl: declaration };

    for (const base of declaration.node.bases ?? []) {
      const parent = resolve(base.type.desugaredQualType ?? base.type.qualType, declaration.scope);
      if (parent) {
        const found = findConstructor(parent, seen);
        if (found) return found;
      }
    }
    if (ctors.length > 0) return { ctor: ctors[0], decl: declaration };
  }

  function extractParameters(declaration: Declaration): ParameterMetadata[] {
    const found = findConstructor(declaration);
    if (!found) return [];
    const { ctor } = found;
    const parms = children(ctor).filter(child => child.kind === 'ParmVarDecl');
    const relevantParms = parms.filter(p => p.name !== 'blockId');
    if (relevantParms.length === 0) return [];

    const comment = fullComment(ctor);
    const paramDocs = new Map<string, {
      name: string;
      description: string;
      icon: string;
      control: Record<string, unknown>;
    }>();

    let currentParam: {
      name: string;
      description: string;
      icon: string;
      control: Record<string, unknown>;
    } | undefined;

    for (const child of children(comment)) {
      if (child.kind === 'ParamCommandComment') {
        const paramId = (child as unknown as { param?: string }).param ?? '';
        const doc = { name: '', description: '', icon: '', control: {} as Record<string, unknown> };

        const innerList = children(child).flatMap(c => c.kind === 'ParagraphComment' ? children(c) : [c]);
        const textParts: string[] = [];
        let seenCommand = false;

        for (let i = 0; i < innerList.length; i++) {
          const item = innerList[i];
          if (item.kind === 'InlineCommandComment') {
            seenCommand = true;
            const cmdName = item.name;
            let valText = '';
            let j = i + 1;
            while (j < innerList.length && innerList[j].kind !== 'InlineCommandComment') {
              if (innerList[j].kind === 'TextComment') {
                valText += innerList[j].text ?? '';
              }
              j++;
            }
            i = j - 1;
            const val = valText.trim();
            if (cmdName === 'icon') {
              doc.icon = val;
            } else if (cmdName === 'control') {
              doc.control.type = val;
            } else if (cmdName) {
              const num = Number(val);
              doc.control[cmdName] = !Number.isNaN(num) && val !== '' ? num : val;
            }
          } else if (item.kind === 'TextComment' && !seenCommand) {
            const text = item.text?.trim();
            if (text) {
              textParts.push(text);
            }
          }
        }

        if (textParts.length > 1) {
          doc.name = textParts[0];
          doc.description = textParts.slice(1).join('\n\n');
        } else if (textParts.length === 1) {
          doc.name = textParts[0];
        }

        currentParam = doc;
        paramDocs.set(paramId, doc);
        continue;
      }
      if (!currentParam) continue;

      if (child.kind === 'BlockCommandComment') {
        if (child.name === 'brief' || child.name === 'details') {
          const text = commentText(child);
          if (text) {
            currentParam.description = currentParam.description
              ? `${currentParam.description}\n\n${text}`
              : text;
          }
        } else if (child.name) {
          const val = commentText(child);
          if (child.name === 'control') {
            currentParam.control.type = val;
          } else {
            const num = Number(val);
            currentParam.control[child.name] = !Number.isNaN(num) && val !== '' ? num : val;
          }
        }
      } else if (child.kind === 'VerbatimLineComment') {
        const loc = child.loc;
        const isImage = !!loc?.file && sources.get(loc.file)?.subarray(loc.offset, loc.offset + loc.tokLen).toString() === 'image';
        if (isImage) {
          currentParam.icon = child.text?.trim().replace(/^(?:html|latex|docbook|rtf|xml)\s+/, '').split(/\s+/, 1)[0] ?? '';
        }
      }
    }

    return relevantParms.map(parm => {
      const id = parm.name ?? '';
      const doc = paramDocs.get(id);
      return {
        id,
        namespace: declaration.scope.join('::'),
        name: doc?.name || id,
        description: doc?.description || '',
        icon: doc?.icon || '',
        control: doc?.control?.type ? doc.control : { type: 'text', ...(doc?.control ?? {}) },
      };
    });
  }

  const blocks: BlockMetadata[] = [];
  for (const declaration of declarations.values()) {
    if (!isBlock(declaration)) continue;
    let template = declaration.template;
    let typeArguments: AstNode[] = [];
    if (!template) {
      // Concrete classes and aliases supply ports through a base or aliased specialization.
      const alias = children(declaration.node).find(child => child.kind === 'TemplateSpecializationType');
      const base = declaration.node.bases?.find(base => {
        const parent = resolve(base.type.desugaredQualType ?? base.type.qualType, declaration.scope);
        return parent && isBlock(parent);
      });
      const type = alias?.type ?? base?.type;
      if (!type) continue;
      const specialization = alias ?? specializations.get(type.desugaredQualType ?? type.qualType);
      if (!specialization) continue;
      template = resolve(type.desugaredQualType ?? type.qualType, declaration.scope)?.template;
      typeArguments = children(specialization).filter(child => child.kind === 'TemplateArgument');
    }
    const parameters = children(template).filter(child => child.kind === 'TemplateTypeParmDecl');
    const parameterType = (index: number) => typeArguments[index]?.type ?? parameters[index]?.defaultArg?.type;
    const inputIndex = parameters.findIndex(parameter => parameter.name === 'I');
    const input = inputIndex !== -1 ? parameterType(inputIndex) : undefined;
    const outputIndex = parameters.findIndex(parameter => parameter.name === 'O');
    const output = outputIndex !== -1 ? parameterType(outputIndex) : undefined;
    // A block may omit I or O when its base fixes the type to void.
    if ((inputIndex === -1 || input) && (outputIndex === -1 || output)) {
      blocks.push({
        ...declaration.meta,
        inputs: input ? ports(input, declaration.scope) : [],
        outputs: output ? ports(output, declaration.scope) : [],
        parameters: extractParameters(declaration),
      });
    }
  }
  const sorted = <T extends { id: string; namespace?: string }>(values: Iterable<T>): T[] => [...values].sort((a, b) => {
    const left = a.namespace === undefined ? a.id : `${a.namespace}::${a.id}`;
    const right = b.namespace === undefined ? b.id : `${b.namespace}::${b.id}`;
    return left < right ? -1 : left > right ? 1 : 0;
  });
  return { namespaces: sorted(namespaces.values()), blocks: sorted(blocks) };
}

if (isMainThread) {
  try {
    if (Number(process.versions.node.split('.')[0]) < 24) throw new Error('Node.js >= 24 is required');
    if (process.argv.length > 3) throw new Error('Usage: node generate-meta.ts [clang-wasm asset directory]');
    const assets = path.resolve(process.argv[2] ?? process.env.CLANG_WASM_DIR ?? path.join(root, '.cache/clang-23.1.2'));
    const worker = new Worker(new URL(import.meta.url), { workerData: { assets } });
    const result = await new Promise<Metadata>((resolve, reject) => {
      let meta: Metadata | undefined;
      worker.on('message', value => { meta = value; });
      worker.on('error', reject);
      worker.on('exit', code => code === 0 && meta ? resolve(meta) : reject(new Error(`clang worker exited with code ${code}`)));
    });
    await mkdir(path.join(root, '.cache'), { recursive: true });
    await writeFile(path.join(root, '.cache/meta.json'), `${JSON.stringify(result, null, 2)}\n`);
    console.log(`Wrote .cache/meta.json: ${result.blocks.length} blocks, ${result.namespaces.length} namespaces`);
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exitCode = 1;
  }
} else {
  if (!parentPort) throw new Error('Missing worker parent port');
  parentPort.postMessage(await compile(workerData.assets));
}
