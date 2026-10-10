/*
 * Copyright 2025 WAFlash-ReFlexed Authors
 * Apache License 2.0
 *
 * Loader for WAFlash-ReFlexed Emscripten module.
 * Uses the Emscripten-generated JS (waflash-reflexed.js) directly
 * instead of manual WebAssembly.instantiate() — which fails because
 * the engine needs ~500 host bindings under namespace "a".
 */

(function() {
  'use strict';

  /**
   * Load the Emscripten JS module dynamically
   */
  function loadScript(src) {
    return new Promise((resolve, reject) => {
      if (document.querySelector(`script[src="${src}"]`)) {
        resolve(); return;
      }
      const s = document.createElement('script');
      s.src = src;
      s.onload = resolve;
      s.onerror = () => reject(new Error('Failed to load: ' + src));
      document.head.appendChild(s);
    });
  }

  /**
   * Write a UTF-8 string into Emscripten heap, return pointer
   */
  function writeString(Module, str) {
    const bytes = new TextEncoder().encode(str + '\0');
    const ptr   = Module._malloc(bytes.length);
    Module.HEAPU8.set(bytes, ptr);
    return ptr;
  }

  /**
   * Main API: createWaflashReflexed(swfUrl, options)
   *
   * options:
   *   gpu           {boolean} — use WebGL renderer (default: true)
   *   enableFilters {boolean} — enable AS display filters (default: true)
   *   canvas        {HTMLCanvasElement} — target canvas (optional)
   *   basePath      {string}  — path to waflash-reflexed.js (default: './')
   */
  async function createWaflashReflexed(swfUrl, options = {}) {
    const basePath = options.basePath || './';
    const jsPath   = basePath + 'waflash-reflexed.js';

    console.log('[WAFlash-ReFlexed] Loading Emscripten module:', jsPath);

    // Configure Emscripten Module BEFORE loading the script
    const canvas = options.canvas ||
                   document.getElementById('flash-canvas') ||
                   document.querySelector('canvas');

    window.Module = {
      canvas: canvas,
      noInitialRun: true,        // we call main() manually with our args
      noExitRuntime: true,
      print:    (t) => console.log('[WASM]', t),
      printErr: (t) => console.warn('[WASM ERR]', t),
      onAbort:  (w) => console.error('[WASM ABORT]', w),
      locateFile: (path) => basePath + path,
    };

    // Load the Emscripten-generated JS (handles all 500 imports)
    await loadScript(jsPath);

    // Wait for runtime to be ready
    await new Promise((resolve) => {
      if (window.Module.calledRun || window.Module.runtimeInitialized) {
        resolve();
      } else {
        const prev = window.Module.onRuntimeInitialized;
        window.Module.onRuntimeInitialized = function() {
          if (prev) prev();
          resolve();
        };
      }
    });

    const M = window.Module;
    console.log('[WAFlash-ReFlexed] Runtime ready');

    // Build argv matching the 4-param schema from architecture_spec.md:
    // argv[0] = "player"
    // argv[1] = swfUrl        (max 4095 bytes)
    // argv[2] = "0"           (subsystem)
    // argv[3] = "webgl"|"default" (renderer)
    // argv[4] = "0"|"1"       (filters: 0=on, 1=off)
    const renderer   = (options.gpu !== false) ? 'webgl' : 'default';
    const filterFlag = (options.enableFilters !== false) ? '0' : '1';

    const argvStrings = ['player', swfUrl, '0', renderer, filterFlag];
    const argc = argvStrings.length;

    // Allocate argv array in WASM heap
    const argvPtr = M._malloc(argc * 4);
    const ptrs = argvStrings.map(s => writeString(M, s));
    ptrs.forEach((ptr, i) => M.HEAP32[(argvPtr >> 2) + i] = ptr);

    console.log('[WAFlash-ReFlexed] Calling _main with:', argvStrings);

    // Call main() — starts the Flash engine
    try {
      M._main(argc, argvPtr);
    } catch (e) {
      // Emscripten throws on exit() — this is normal
      if (!String(e).includes('ExitStatus')) throw e;
    }

    // Clean up argv pointers
    ptrs.forEach(ptr => M._free(ptr));
    M._free(argvPtr);

    // Start frame loop
    let running = true;
    function tick() {
      if (!running) return;
      try {
        if (M._engine_tick) M._engine_tick();
      } catch(e) {
        console.warn('[WAFlash-ReFlexed] tick error:', e);
      }
      requestAnimationFrame(tick);
    }
    requestAnimationFrame(tick);

    console.log('[WAFlash-ReFlexed] Engine running ▶');

    return {
      play()  { try { M._Play();  } catch(e) {} },
      stop()  { try { M._Stop();  } catch(e) {} },
      get memory() { return M.HEAP8.buffer; },
      destroy() {
        running = false;
        console.log('[WAFlash-ReFlexed] Destroyed');
      }
    };
  }

  // Export
  if (typeof module !== 'undefined' && module.exports) {
    module.exports = { createWaflashReflexed };
  } else {
    window.createWaflashReflexed = createWaflashReflexed;
  }

})();
