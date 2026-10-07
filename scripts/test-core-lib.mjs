import assert from 'node:assert/strict';
import {execFileSync, spawnSync} from 'node:child_process';
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
    const diagramTraps = ['diagram_scalar_index', 'diagram_connection_overflow',
        'diagram_group_index', 'diagram_output_width', 'diagram_gpio_width'];
    execFileSync(compiler, [...flags,
        path.join(root, 'tests/freestanding/blocks.cpp'), ...linkFlags,
        '-Wl,--export=block_checks', '-Wl,--export=diagram_checks',
        ...diagramTraps.map(name => `-Wl,--export=${name}`), '-o', blocksWasm,
    ], {stdio: 'inherit'});
    const blocksModule = await WebAssembly.compile(fs.readFileSync(blocksWasm));
    assert.deepEqual(WebAssembly.Module.imports(blocksModule).map(({module, name}) => `${module}.${name}`).sort(),
        ['env.cos', 'env.fmod', 'env.sin']);
    const blocks = await WebAssembly.instantiate(blocksModule, {env: {
        sin: Math.sin, cos: Math.cos, fmod: (value, divisor) => value % divisor,
    }});
    for (let i = 0; i < 1000; ++i)
        assert.equal(blocks.exports.block_checks(), 0, 'Block check failed at this source line');
    for (let i = 0; i < 1000; ++i)
        assert.equal(blocks.exports.diagram_checks(), 0, 'Diagram check failed at this source line');
    for (const name of diagramTraps)
        assert.throws(() => blocks.exports[name](), WebAssembly.RuntimeError, `${name} must trap`);
    console.log('Block Wasm checks passed: channel storage, shared state, math imports, timers, and GPIO.');
    const registeredWasm = path.join(temporary, 'diagram-registered.wasm');
    execFileSync(compiler, [...flags, '-DTEST_GPIO_REGISTRATION',
        path.join(root, 'tests/freestanding/blocks.cpp'), ...linkFlags,
        '-Wl,--export=diagram_checks', '-o', registeredWasm,
    ], {stdio: 'inherit'});
    const registeredModule = await WebAssembly.compile(fs.readFileSync(registeredWasm));
    assert.deepEqual(WebAssembly.Module.imports(registeredModule), [], 'The diagram API needs no new imports');
    const registered = await WebAssembly.instantiate(registeredModule);
    for (let i = 0; i < 1000; ++i)
        assert.equal(registered.exports.diagram_checks(), 0, 'GPIO registration check failed at this source line');
    console.log('Diagram Wasm checks passed: uniform wiring, defaults, fanout, bounds, and optional GPIO registration.');
    const rejected = [
        ['precision', `
            core::function<void(float)>* output = nullptr;
            core::span<core::function<void(double)>* const> input;
            auto connections = core::input_connections<true, 1, 1>(input);
            connections.connect(0, output);`, /cannot initialize a parameter/],
        ['count', `
            VectorizedOutput<int> port;
            auto channels = core::output_channels<true, 256>(port);`, /channel count does not fit/],
        ['scalar', `
            int port = 0;
            auto connections = core::input_connections<false, 2, 1>(port);`, /Scalar input.*one connection/],
    ];
    for (const [name, body, diagnostic] of rejected) {
        const source = path.join(temporary, `${name}.cpp`);
        fs.writeFileSync(source, `#include <core/diagram.hpp>\nvoid check() { ${body} }\n`);
        const result = spawnSync(compiler, [...flags.filter(flag => flag !== '-nostdlib'),
            '-fsyntax-only', source], {encoding: 'utf8'});
        assert.notEqual(result.status, 0, `${name} must fail compilation`);
        assert.match(result.stderr, diagnostic);
    }
    console.log('Diagram compile checks passed: incompatible stream types, channel count overflow, and scalar fan-in rejected.');
} finally {
    fs.rmSync(temporary, {recursive: true, force: true});
}
