#!/usr/bin/env node
// Node >= 24. Usage: node .github/scripts/generate-meta.mjs [clang-wasm asset directory]
import { mkdir, readFile, readdir, writeFile } from 'node:fs/promises';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { gunzipSync } from 'node:zlib';
import { Worker, isMainThread, parentPort, workerData } from 'node:worker_threads';

const root = fileURLToPath(new URL('../../', import.meta.url));

// This release supports web/worker environments only. Keep its browser shim in
// a worker so the main Node environment remains intact; use the original JS glue.
async function compile(assets) {
  const wasm = await readFile(path.join(assets, 'clang.wasm'));
  const archive = await readFile(path.join(assets, 'sysroot.tgz'));
  const { default: createClang } = await import(pathToFileURL(path.join(assets, 'clang.js')));
  globalThis.process = undefined;
  globalThis.window = {};
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

  const sources = new Map();
  async function copyHeaders(directory) {
    const entries = await readdir(path.join(root, 'src', directory), { withFileTypes: true });
    entries.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0);
    for (const entry of entries) {
      const relative = path.posix.join(directory, entry.name);
      if (entry.isDirectory()) {
        await copyHeaders(relative);
      } else if (entry.isFile() && entry.name.endsWith('.hpp')) {
        const filename = `/project/src/${relative}`;
        const content = await readFile(path.join(root, 'src', relative));
        FS.mkdirTree(path.posix.dirname(filename));
        FS.writeFile(filename, content);
        sources.set(filename, content);
      }
    }
  }
  await copyHeaders('');
  if (!sources.size) throw new Error('No .hpp files found under src/');
  // Include the public entry point first, then cover headers it does not include.
  const headers = [...sources.keys()].sort();
  const entry = '/project/src/base.hpp';
  if (sources.has(entry)) {
    headers.splice(headers.indexOf(entry), 1);
    headers.unshift(entry);
  }
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
    '-isystem', '/sysroot/lib/clang/23/include', '-isystem', '/sysroot/include',
    '-I', '/project/src', '-fparse-all-comments', '-ast-dump=json', '/project/meta.cpp',
  ]);
  FS.close(stdout);
  if (status !== 0) throw new Error(`clang failed with exit code ${status}`);
  return extractMetadata(JSON.parse(FS.readFile('/ast.json', { encoding: 'utf8' })), sources);
}

// Extract the release's ustar archive straight into clang's in-memory filesystem.
// No host tar executable, host sysroot, or npm packages are required.
function installSysroot(FS, archive) {
  const tar = gunzipSync(archive);
  const string = bytes => bytes.toString('utf8').split('\0', 1)[0];
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

function extractMetadata(ast, sources) {
  // Clang omits filenames when they match the preceding emitted location.
  // Restore them in JSON property order, excluding include-stack locations.
  let lastFile;
  function restoreFiles(value) {
    if (!value || typeof value !== 'object') return;
    if ('offset' in value && 'col' in value) {
      if (value.file) lastFile = value.file;
      else value.file = lastFile;
    }
    for (const [key, child] of Object.entries(value)) {
      if (key !== 'includedFrom') restoreFiles(child);
    }
  }
  restoreFiles(ast);

  const children = node => node.inner ?? [];
  const fullComment = node => children(node).find(child => child.kind === 'FullComment');
  const commentText = node => [node.text ?? '', ...children(node).map(commentText)]
    .join(' ').replace(/\s+/g, ' ').trim();
  function metadata(node, scope, comment = fullComment(node)) {
    const parts = children(comment ?? {});
    const paragraphs = parts.filter(part => part.kind === 'ParagraphComment').map(commentText).filter(Boolean);
    const descriptions = parts.filter(part => part.kind === 'BlockCommandComment' &&
      ['brief', 'details'].includes(part.name)).map(commentText);
    // JSON omits the command name on VerbatimLineComment, so consult its source
    // location to distinguish @image from other verbatim commands such as @file.
    const image = parts.find(part => {
      if (part.kind !== 'VerbatimLineComment') return false;
      const loc = part.loc;
      return sources.get(loc?.file)?.subarray(loc.offset, loc.offset + loc.tokLen).toString() === 'image';
    });
    const icon = image?.text.trim().replace(/^(?:html|latex|docbook|rtf|xml)\s+/, '').split(/\s+/, 1)[0] ?? '';
    return {
      id: node.name,
      namespace: scope.join('::'),
      name: paragraphs[0] ?? node.name,
      description: [...descriptions, ...paragraphs.slice(1)].filter(Boolean).join('\n\n'),
      icon,
    };
  }

  const declarations = new Map();
  const namespaces = new Map();
  const qualified = (scope, name) => [...scope, name].join('::');
  function visit(node, scope = [], template) {
    if (node.isImplicit || node.kind.includes('Specialization')) return;
    const local = sources.has(node.loc?.file);
    if (node.kind === 'NamespaceDecl') {
      if (!node.name) return;
      if (local) {
        const key = qualified(scope, node.name);
        if (fullComment(node) || !namespaces.has(key)) {
          const { name, description, icon } = metadata(node, scope);
          namespaces.set(key, { id: key, name, description, icon });
        }
      }
      for (const child of children(node)) visit(child, [...scope, node.name]);
    } else if (node.kind === 'ClassTemplateDecl' || node.kind === 'TypeAliasTemplateDecl') {
      for (const child of children(node)) {
        if (['CXXRecordDecl', 'TypeAliasDecl'].includes(child.kind)) visit(child, scope, node);
      }
    } else if (['CXXRecordDecl', 'RecordDecl', 'EnumDecl', 'TypeAliasDecl', 'TypedefDecl'].includes(node.kind)) {
      if (!local || !node.name) return;
      if (node.kind.endsWith('RecordDecl') && !node.completeDefinition) return;
      declarations.set(qualified(scope, node.name), {
        node, scope, template, meta: metadata(node, scope, fullComment(template ?? node) ?? fullComment(node)),
      });
      if (node.kind.endsWith('RecordDecl')) {
        for (const child of children(node)) visit(child, [...scope, node.name]);
      }
    } else if (['TranslationUnitDecl', 'LinkageSpecDecl', 'ExportDecl'].includes(node.kind)) {
      for (const child of children(node)) visit(child, scope);
    }
  }
  visit(ast);

  function resolve(type, scope) {
    const name = type.replace(/<.*>$/, '').replace(/^::/, '');
    for (let length = type.startsWith('::') ? 0 : scope.length; length >= 0; length--) {
      const found = declarations.get(qualified(scope.slice(0, length), name));
      if (found) return found;
    }
  }
  function isBlock(declaration, seen = new Set()) {
    if (seen.has(declaration)) return false;
    seen.add(declaration);
    if (declaration.node.name === 'Block' && !declaration.scope.length) return true;
    return (declaration.node.bases ?? []).some(base => {
      const parent = resolve(base.type.desugaredQualType ?? base.type.qualType, declaration.scope);
      return parent && isBlock(parent, seen);
    });
  }
  function ports(type, scope) {
    const spelling = type.desugaredQualType ?? type.qualType;
    if (spelling === 'void') return [];
    const declaration = resolve(spelling, scope);
    if (!declaration) throw new Error(`Cannot resolve port struct ${spelling} in ${scope.join('::')}`);
    if (['TypeAliasDecl', 'TypedefDecl'].includes(declaration.node.kind)) {
      return ports(declaration.node.type, declaration.scope);
    }
    if (!declaration.node.completeDefinition) throw new Error(`Incomplete port struct: ${spelling}`);
    const inherited = (declaration.node.bases ?? []).flatMap(base => ports(base.type, declaration.scope));
    const fields = children(declaration.node).filter(child => child.kind === 'FieldDecl' && !child.isImplicit);
    return [...inherited, ...fields.map(field => metadata(field, declaration.scope))];
  }

  const types = [];
  const blocks = [];
  for (const declaration of declarations.values()) {
    const parameters = children(declaration.template ?? {}).filter(child => child.kind === 'TemplateTypeParmDecl');
    const input = parameters.find(parameter => parameter.name === 'I')?.defaultArg?.type;
    const output = parameters.find(parameter => parameter.name === 'O')?.defaultArg?.type;
    // The library's usable block templates provide defaults for both I and O.
    if (input && output && isBlock(declaration)) {
      blocks.push({ ...declaration.meta,
        inputs: ports(input, declaration.scope), outputs: ports(output, declaration.scope) });
    } else if (declaration.node.loc.file === '/project/src/core/types.hpp') {
      types.push(declaration.meta);
    }
  }
  const sorted = values => [...values].sort((a, b) => {
    const left = a.namespace === undefined ? a.id : `${a.namespace}::${a.id}`;
    const right = b.namespace === undefined ? b.id : `${b.namespace}::${b.id}`;
    return left < right ? -1 : left > right ? 1 : 0;
  });
  return { namespaces: sorted(namespaces.values()), types: sorted(types), blocks: sorted(blocks) };
}

if (isMainThread) {
  try {
    if (Number(process.versions.node.split('.')[0]) < 24) throw new Error('Node.js >= 24 is required');
    if (process.argv.length > 3) throw new Error('Usage: node generate-meta.mjs [clang-wasm asset directory]');
    const assets = path.resolve(process.argv[2] ?? process.env.CLANG_WASM_DIR ?? path.join(root, '.cache/clang-23.1.2'));
    const worker = new Worker(new URL(import.meta.url), { workerData: { assets } });
    const result = await new Promise((resolve, reject) => {
      let meta;
      worker.on('message', value => { meta = value; });
      worker.on('error', reject);
      worker.on('exit', code => code === 0 && meta ? resolve(meta) : reject(new Error(`clang worker exited with code ${code}`)));
    });
    await mkdir(path.join(root, '.cache'), { recursive: true });
    await writeFile(path.join(root, '.cache/meta.json'), `${JSON.stringify(result, null, 2)}\n`);
    console.log(`Wrote .cache/meta.json: ${result.types.length} types, ${result.blocks.length} blocks, ${result.namespaces.length} namespaces`);
  } catch (error) {
    console.error(error.message);
    process.exitCode = 1;
  }
} else {
  parentPort.postMessage(await compile(workerData.assets));
}
