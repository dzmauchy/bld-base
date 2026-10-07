import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const root = path.resolve(import.meta.dirname, '..');
const sysroot = path.resolve(process.argv[2] ?? path.join(root, '../clang-wasm/dist/sysroot'));
const compiler = process.argv[3] ?? 'clang++';
const resourceRoot = path.join(sysroot, 'lib/clang');
const versions = fs.readdirSync(resourceRoot);
assert.equal(versions.length, 1, 'Expected one Clang resource version in the sysroot');
const resource = path.join(resourceRoot, versions[0]);
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'base-core-lib-'));

try {
    const wasm = path.join(temporary, 'core-lib.wasm');
    const flags = [
        '--target=wasm32-unknown-unknown', '-std=c++23', '-O2',
        '-Wall', '-Wextra', '-Wpedantic', '-Werror',
        '-ffreestanding', '-nostdinc++', '-nostdlib', '-fno-exceptions',
        '-fno-rtti', '-fno-threadsafe-statics', `--sysroot=${sysroot}`,
        `-resource-dir=${resource}`, `-I${path.join(root, 'src')}`,
    ];
    const linkFlags = [
        `-L${path.join(resource, 'lib/wasi')}`, '-lclang_rt.builtins-wasm32',
        '-Wl,--no-entry',
    ];
    execFileSync(compiler, [...flags,
        path.join(root, 'tests/freestanding/core_lib.cpp'), ...linkFlags,
        '-Wl,--export=core_lib_checks',
        '-Wl,--export=core_lib_empty_function', '-Wl,--export=core_lib_invalid_index',
        '-Wl,--export=core_lib_size_overflow', '-o', wasm,
    ], {stdio: 'inherit'});
    const module = await WebAssembly.compile(fs.readFileSync(wasm));
    assert.deepEqual(WebAssembly.Module.imports(module), [], 'Core types need no host imports');
    const instance = await WebAssembly.instantiate(module);
    for (let i = 0; i < 1000; ++i)
        assert.equal(instance.exports.core_lib_checks(), 0, 'Freestanding check failed at this source line');
    for (const name of ['core_lib_empty_function', 'core_lib_invalid_index', 'core_lib_size_overflow'])
        assert.throws(() => instance.exports[name](), WebAssembly.RuntimeError, `${name} must trap`);
    console.log('Core lib Wasm checks passed: fixed-size arrays, TLSF ownership, copies, alignment, and traps.');
    const blocksWasm = path.join(temporary, 'blocks.wasm');
    execFileSync(compiler, [...flags,
        path.join(root, 'tests/freestanding/blocks.cpp'), ...linkFlags,
        '-Wl,--export=block_checks', '-o', blocksWasm,
    ], {stdio: 'inherit'});
    const blocksModule = await WebAssembly.compile(fs.readFileSync(blocksWasm));
    assert.deepEqual(WebAssembly.Module.imports(blocksModule).map(({module, name}) => `${module}.${name}`).sort(),
        ['env.cos', 'env.fmod', 'env.sin']);
    const blocks = await WebAssembly.instantiate(blocksModule, {env: {
        sin: Math.sin, cos: Math.cos, fmod: (value, divisor) => value % divisor,
    }});
    for (let i = 0; i < 1000; ++i)
        assert.equal(blocks.exports.block_checks(), 0, 'Block check failed at this source line');
    console.log('Block Wasm checks passed: channel storage, shared state, math imports, timers, and GPIO.');
} finally {
    fs.rmSync(temporary, {recursive: true, force: true});
}
