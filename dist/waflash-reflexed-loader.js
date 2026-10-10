/*
 * Copyright 2025 WAFlash-ReFlexed Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

async function createWaflashReflexed(swfUrl, options = {}) {
    const memory = new WebAssembly.Memory({ initial: 256, maximum: 32768 });
    const importObject = {
        env: {
            memory: memory,
            abort: () => { console.error("WASM Aborted"); }
        },
        a: {
            memory: memory
        }
    };

    let wasmModule;
    if (typeof WebAssembly.instantiateStreaming === 'function') {
        try {
            const response = await fetch('waflash-reflexed.wasm');
            const result = await WebAssembly.instantiateStreaming(response, importObject);
            wasmModule = result.instance;
        } catch (e) {
            const response = await fetch('waflash-reflexed.wasm');
            const bytes = await response.arrayBuffer();
            const result = await WebAssembly.instantiate(bytes, importObject);
            wasmModule = result.instance;
        }
    } else {
        const response = await fetch('waflash-reflexed.wasm');
        const bytes = await response.arrayBuffer();
        const result = await WebAssembly.instantiate(bytes, importObject);
        wasmModule = result.instance;
    }

    const { _main, _Play, _Stop, _engine_tick, _malloc, _free } = wasmModule.exports;

    const renderer = options.gpu ? "webgl" : "default";
    const filterFlag = options.enableFilters ? "0" : "1";

    const argvStrings = [
        "player",
        swfUrl,
        "0",
        renderer,
        filterFlag
    ];

    if (_main && _malloc) {
        const ptrSize = 4;
        const argvPtr = _malloc(argvStrings.length * ptrSize);
        const heap32 = new Int32Array(memory.buffer);

        for (let i = 0; i < argvStrings.length; i++) {
            const str = argvStrings[i];
            const encoder = new TextEncoder();
            const bytes = encoder.encode(str + '\0');
            const strPtr = _malloc(bytes.length);
            const heap8 = new Uint8Array(memory.buffer);
            heap8.set(bytes, strPtr);
            heap32[(argvPtr / ptrSize) + i] = strPtr;
        }

        _main(argvStrings.length, argvPtr);
    }

    // Frame loop
    function tick() {
        if (typeof _engine_tick === 'function') {
            _engine_tick();
        }
        if (typeof requestAnimationFrame === 'function') {
            requestAnimationFrame(tick);
        }
    }
    if (typeof requestAnimationFrame === 'function') {
        requestAnimationFrame(tick);
    }

    return {
        play: () => { if (_Play) _Play(); },
        stop: () => { if (_Stop) _Stop(); },
        memory: memory
    };
}

if (typeof module !== 'undefined' && module.exports) {
    module.exports = { createWaflashReflexed };
}
