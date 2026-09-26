import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { copyFile, mkdir, mkdtemp, readFile, rm, writeFile } from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import type { BlockMetadata, Metadata, MetadataEntry } from './generate-meta.ts';

const root = path.resolve(import.meta.dirname, '..');
const script = path.join(root, 'scripts/generate-meta.ts');
const assets = path.resolve(process.env.CLANG_WASM_DIR ?? path.join(root, '.cache/clang-23.1.2'));
const run = (filename: string) => spawnSync(process.execPath, [filename, assets], { cwd: os.tmpdir(), encoding: 'utf8' });

test('generates all library ports, documentation, and only the requested metadata', async () => {
  const result = run(script);
  assert.equal(result.status, 0, result.stderr);
  const generated = await readFile(path.join(root, '.cache/meta.json'), 'utf8');
  const meta: Metadata = JSON.parse(generated);
  assert.deepEqual(Object.keys(meta).sort(), ['blocks', 'namespaces']);
  assert.equal(meta.blocks.length, 22);
  assert.equal(meta.namespaces.length, 10);
  assert.deepEqual(meta.namespaces.map(entry => entry.id), [
    'math', 'push', 'push::f_32', 'push::f_32::sinks', 'push::f_32::sources', 'push::f_32::transformers',
    'push::f_64', 'push::f_64::sinks', 'push::f_64::sources', 'push::f_64::transformers',
  ]);
  const entryKeys = ['description', 'icon', 'id', 'name', 'namespace'] as const;
  for (const category of ['namespaces', 'blocks'] as const) {
    const keys = category === 'namespaces' ? entryKeys.filter(key => key !== 'namespace') : entryKeys;
    const entries: (Omit<MetadataEntry, 'namespace'> & Partial<BlockMetadata>)[] = meta[category];
    const ids = entries.map(entry => category === 'namespaces' ? entry.id : `${entry.namespace}::${entry.id}`);
    assert.equal(new Set(ids).size, ids.length);
    for (const entry of entries) {
      assert.deepEqual(Object.keys(entry).sort(), category === 'blocks'
        ? [...keys, 'inputs', 'outputs'].sort() : keys);
      for (const key of keys) assert.equal(typeof entry[key], 'string');
      for (const port of [...(entry.inputs ?? []), ...(entry.outputs ?? [])]) {
        assert.deepEqual(Object.keys(port).sort(), entryKeys);
        assert.ok(port.name && port.description && port.icon);
      }
    }
  }
  const expected = {
    Cos: [['downstream'], ['consumer']], Sin: [['downstream'], ['consumer']],
    Product: [['downstream', 'channelCount'], ['channels']],
    Sum: [['downstream', 'channelCount'], ['channels']],
    Scope: [['channelCount'], ['channels']], GpioIn: [['pins'], []],
    Const: [['downstream'], []], CosGen: [['downstream'], []],
    SinGen: [['downstream'], []], RandGen: [['downstream'], []], PulseGen: [['downstream'], []],
  };
  for (const precision of ['F32', 'F64']) {
    for (const [name, ports] of Object.entries(expected)) {
      const block = meta.blocks.find(entry => entry.id === name + precision);
      assert.ok(block, name + precision);
      assert.deepEqual([block.inputs.map(port => port.id), block.outputs.map(port => port.id)], ports);
    }
  }
  const cosine = meta.blocks.find(entry => entry.id === 'CosF32');
  assert.ok(cosine);
  assert.equal(cosine.name, 'cos');
  assert.equal(cosine.icon, 'cos.svg');
  assert.equal(cosine.description, 'Computes the cosine of the input value');
  assert.equal(cosine.outputs[0].name, 'Cosine input consumer');
  assert.equal(run(script).status, 0);
  assert.equal(await readFile(path.join(root, '.cache/meta.json'), 'utf8'), generated);
});

test('reads unincluded headers, UTF-8 comments, and multiple fields; preserves output on compiler failure', async () => {
  const temporary = await mkdtemp(path.join(os.tmpdir(), 'bld-meta-'));
  try {
    await mkdir(path.join(temporary, 'scripts'), { recursive: true });
    await mkdir(path.join(temporary, 'src/core'), { recursive: true });
    const fixtureScript = path.join(temporary, 'scripts/generate-meta.ts');
    await copyFile(script, fixtureScript);
    await writeFile(path.join(temporary, 'src/blocks.hpp'), `
template <typename I, typename O> class Block {};
namespace example {
/** Entrée
 * @brief Values supplied by callers.
 * @image input.svg
 */
struct In {
  /** Première valeur
   * @brief First input.
   * @image first.svg
   */
  int first;
  int second;
};
struct Out { int result; int extra; };
using Output = Out;
template <typename I = In, typename O = Output>
class Example : public Block<I, O> {};
template <typename I = In>
class Source : public Block<I, void> {};
}
`);
    await writeFile(path.join(temporary, 'src/extra.hpp'), '/** Additional type */\nusing Extra = double;\n');
    await writeFile(path.join(temporary, 'src/core/types.hpp'), '/** Exported type */\nusing Scalar = double;\n');
    const result = run(fixtureScript);
    assert.equal(result.status, 0, result.stderr);
    const output = path.join(temporary, '.cache/meta.json');
    await assert.rejects(readFile(path.join(temporary, 'meta.json')), { code: 'ENOENT' });
    const generated = await readFile(output, 'utf8');
    const meta: Metadata = JSON.parse(generated);
    assert.deepEqual(Object.keys(meta).sort(), ['blocks', 'namespaces']);
    assert.equal(meta.blocks.length, 2);
    assert.deepEqual(meta.blocks[0].inputs.map(port => port.id), ['first', 'second']);
    assert.deepEqual(meta.blocks[0].outputs.map(port => port.id), ['result', 'extra']);
    assert.equal(meta.blocks[0].inputs[0].name, 'Première valeur');
    assert.equal(meta.blocks[0].inputs[0].icon, 'first.svg');
    const source = meta.blocks.find(entry => entry.id === 'Source');
    assert.ok(source);
    assert.deepEqual(source.outputs, []);
    await writeFile(path.join(temporary, 'src/extra.hpp'), '#error deliberate compiler failure\n');
    const failed = run(fixtureScript);
    assert.notEqual(failed.status, 0);
    assert.match(failed.stderr, /deliberate compiler failure/);
    assert.equal(await readFile(output, 'utf8'), generated);
  } finally {
    await rm(temporary, { recursive: true, force: true });
  }
});
