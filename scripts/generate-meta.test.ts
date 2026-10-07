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
    'core', 'push', 'push::f_32', 'push::f_32::sinks', 'push::f_32::sources', 'push::f_32::transformers',
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
        ? [...keys, 'inputs', 'outputs', 'parameters'].sort() : keys);
      for (const key of keys) assert.equal(typeof entry[key], 'string');
      for (const port of [...(entry.inputs ?? []), ...(entry.outputs ?? [])]) {
        assert.deepEqual(Object.keys(port).sort(), port.vectorized ? [...entryKeys, 'vectorized'].sort() : entryKeys);
        if ('vectorized' in port) assert.equal(port.vectorized, true);
        assert.ok(port.name && port.description && port.icon);
      }
      for (const parameter of entry.parameters ?? []) {
        assert.deepEqual(Object.keys(parameter).sort(), [...entryKeys, 'control'].sort());
        assert.ok(parameter.name && parameter.description && parameter.icon);
        assert.equal(typeof parameter.control, 'object');
        assert.ok(parameter.control && typeof parameter.control.type === 'string');
      }
    }
  }
  const expected = {
    Cos: [[['downstream'], ['consumer']], []],
    Sin: [[['downstream'], ['consumer']], []],
    Product: [[['downstream'], ['channels']], ['precision']],
    Sum: [[['downstream'], ['channels']], ['precision']],
    Scope: [[[], ['channels']], ['period', 'precision']],
    GpioIn: [[['pins'], []], ['port', 'pins']],
    Const: [[['downstream'], []], ['value']],
    CosGen: [[['downstream'], []], ['precision', 'frequency', 'amplitude', 'phase']],
    SinGen: [[['downstream'], []], ['precision', 'frequency', 'amplitude', 'phase']],
    RandGen: [[['downstream'], []], ['precision', 'amplitude']],
    PulseGen: [[['downstream'], []], ['dutyCycle', 'amplitude', 'frequency', 'phase']],
  };
  for (const precision of ['F32', 'F64']) {
    for (const [name, [ports, parameters]] of Object.entries(expected)) {
      const block = meta.blocks.find(entry => entry.id === name + precision);
      assert.ok(block, name + precision);
      assert.deepEqual([block.inputs.map(port => port.id), block.outputs.map(port => port.id)], ports);
      assert.ok(block.inputs.every(port => port.vectorized === true), `${block.id} inputs`);
      for (const port of block.outputs) {
        if (port.id === 'channels') assert.equal(port.vectorized, true, `${block.id}.${port.id}`);
        else assert.ok(!('vectorized' in port), `${block.id}.${port.id}`);
      }
      assert.deepEqual(block.parameters.map(parameter => parameter.id), parameters);
    }
  }
  const cosine = meta.blocks.find(entry => entry.id === 'CosF32');
  assert.ok(cosine);
  assert.equal(cosine.name, 'cos');
  assert.equal(cosine.icon, 'cos.svg');
  assert.equal(cosine.description, 'Computes the cosine of the input value');
  assert.equal(cosine.outputs[0].name, 'Cosine input consumer');
  const scope = meta.blocks.find(entry => entry.id === 'ScopeF32');
  assert.ok(scope);
  assert.equal(scope.parameters.length, 2);
  assert.equal(scope.parameters[0].id, 'period');
  assert.equal(scope.parameters[0].name, 'Period');
  assert.equal(scope.parameters[0].icon, 'period.svg');
  assert.deepEqual(scope.parameters[0].control, { type: 'slider', min: 1, max: 3600, step: 1 });
  assert.equal(scope.parameters[1].id, 'precision');
  assert.equal(scope.parameters[1].name, 'Precision');
  assert.equal(scope.parameters[1].icon, 'precision.svg');
  assert.deepEqual(scope.parameters[1].control, { type: 'number', min: 1, max: 1000, step: 1 });
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
    await copyFile(path.join(root, 'src/core/lib.hpp'), path.join(temporary, 'src/core/lib.hpp'));
    await writeFile(path.join(temporary, 'src/blocks.hpp'), `
#include <core/lib.hpp>
template <typename T> using VectorizedInput = core::span<T* const>;
template <typename T> using VectorizedOutput = core::function<core::span<T* const>(unsigned char)>;
namespace example {
using InputChannels = VectorizedInput<int>;
typedef InputChannels ChannelAlias;
using OutputChannels = VectorizedOutput<int>;
struct InBase { ChannelAlias inherited; };
/** Entrée
 * @brief Values supplied by callers.
 * @image input.svg
 */
struct In : InBase {
  /** Première valeur
   * @brief First input.
   * @image first.svg
   */
  int first;
  int second;
  InputChannels channels;
  core::array<ChannelAlias> groups;
  core::array<int*> raw;
  core::span<int* const> view;
  core::span<int> scalarView;
  core::array<int> values;
};
struct Out {
  using LocalChannels = OutputChannels;
  int result;
  int extra;
  LocalChannels channels;
  VectorizedOutput<int> direct;
  core::function<core::array<int*>(unsigned char)> raw;
  core::function<core::span<int* const>(unsigned char)> view;
  core::function<void(VectorizedInput<int>)> callback;
};
using Output = Out;
/** Example
 * @param gain Gain
 *   Scales the values.
 *   @icon gain.svg
 *   @control number
 *   @min 0
 */
inline core::function<Output(In)> Example(unsigned blockId, int gain = 1) { return {}; }
using SourceCallable = core::function<void(In)>;
typedef SourceCallable SourceFunction;
inline auto Source(unsigned blockId) -> SourceFunction { return {}; }
// Helpers and state classes do not describe public factories.
struct State {};
inline int helper(int value) { return value; }
inline core::function<int()> scalarResult() { return {}; }
inline core::function<void(int)> scalarInput() { return {}; }
inline core::function<Out(In, In)> multipleArguments() { return {}; }
inline core::function<core::array<int*>()> channelBuilder() { return {}; }
inline core::function<Out()> Sink(unsigned blockId) { return {}; }
namespace detail {
inline core::function<Output(In)> Internal(unsigned blockId) { return {}; }
}
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
    assert.equal(meta.blocks.length, 3);
    assert.deepEqual(meta.blocks[0].inputs.map(port => port.id), ['inherited', 'first', 'second', 'channels', 'groups', 'raw', 'view', 'scalarView', 'values']);
    assert.deepEqual(meta.blocks[0].outputs.map(port => port.id), ['result', 'extra', 'channels', 'direct', 'raw', 'view', 'callback']);
    assert.deepEqual(meta.blocks[0].inputs.filter(port => port.vectorized).map(port => port.id), ['inherited', 'channels', 'groups', 'raw', 'view']);
    assert.deepEqual(meta.blocks[0].outputs.filter(port => port.vectorized).map(port => port.id), ['channels', 'direct', 'raw', 'view']);
    for (const port of [...meta.blocks[0].inputs, ...meta.blocks[0].outputs]) {
      if (port.vectorized) assert.equal(port.vectorized, true);
      else assert.ok(!('vectorized' in port));
    }
    const first = meta.blocks[0].inputs.find(port => port.id === 'first');
    assert.ok(first);
    assert.equal(first.name, 'Première valeur');
    assert.equal(first.icon, 'first.svg');
    const source = meta.blocks.find(entry => entry.id === 'Source');
    assert.ok(source);
    assert.deepEqual(source.outputs, []);
    const sink = meta.blocks.find(entry => entry.id === 'Sink');
    assert.ok(sink);
    assert.deepEqual(sink.inputs, []);
    assert.deepEqual(sink.outputs.map(port => port.id), meta.blocks[0].outputs.map(port => port.id));
    const example = meta.blocks.find(entry => entry.id === 'Example');
    assert.ok(example);
    assert.deepEqual(example.parameters.map(parameter => parameter.id), ['gain']);
    assert.deepEqual(example.parameters[0].control, { type: 'number', min: 0 });
    assert.ok(!meta.namespaces.some(entry => entry.id.includes('detail')));
    await writeFile(path.join(temporary, 'src/extra.hpp'), '#error deliberate compiler failure\n');
    const failed = run(fixtureScript);
    assert.notEqual(failed.status, 0);
    assert.match(failed.stderr, /deliberate compiler failure/);
    assert.equal(await readFile(output, 'utf8'), generated);
  } finally {
    await rm(temporary, { recursive: true, force: true });
  }
});
