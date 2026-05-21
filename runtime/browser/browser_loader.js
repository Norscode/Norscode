// Norscode browser WASM loader

export async function loadNorscodeModule(path, imports = {}) {
    const runtimeImports = {
        env: {
            memory: imports.memory,
            console_log: imports.console_log || ((value) => {
                console.log('[norscode]', value);
            })
        }
    };

    const response = await fetch(path);
    const bytes = await response.arrayBuffer();

    const result = await WebAssembly.instantiate(bytes, runtimeImports);

    return {
        instance: result.instance,
        module: result.module,
        exports: result.instance.exports
    };
}

export async function runNorscode(path) {
    const runtime = await loadNorscodeModule(path, {
        console_log(value) {
            console.log('[norscode-runtime]', value);
        }
    });

    if (runtime.exports.main) {
        return runtime.exports.main();
    }

    console.warn('No exported main() function found');

    return null;
}
