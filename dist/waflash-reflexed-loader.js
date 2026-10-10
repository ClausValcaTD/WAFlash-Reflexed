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

    // ─── CRITICAL: Fetch SWF and write to MEMFS ───────────────
    // Determine the virtual path inside MEMFS
    // Use a simple flat path — no subdirectories needed
    const swfFilename = swfUrl.split('/').pop() || 'game.swf';
    const memfsPath   = '/waflashso/' + swfFilename;

    console.log('[WAFlash-ReFlexed] Fetching SWF:', swfUrl);
    const response = await fetch(swfUrl);
    if (!response.ok) {
        throw new Error(`Failed to fetch SWF: ${response.status} ${swfUrl}`);
    }

    const swfBytes = new Uint8Array(await response.arrayBuffer());
    console.log('[WAFlash-ReFlexed] SWF fetched:', swfBytes.length, 'bytes');

    // Write to Emscripten MEMFS
    // MEMFS is mounted at /waflashso by our engine
    try {
        M.FS.writeFile(memfsPath, swfBytes);
        console.log('[WAFlash-ReFlexed] SWF written to MEMFS:', memfsPath);
    } catch(e) {
        // /waflashso might not exist yet if _main hasn't run
        // Write to root instead
        try {
            M.FS.writeFile('/' + swfFilename, swfBytes);
            console.log('[WAFlash-ReFlexed] SWF written to MEMFS: /' + swfFilename);
        } catch(e2) {
            console.warn('[WAFlash-ReFlexed] MEMFS write failed:', e2);
        }
    }
    // ──────────────────────────────────────────────────────────

    // Build argv — pass the MEMFS path to _main, not the HTTP URL
    const renderer   = (options.gpu !== false) ? 'webgl' : 'default';
    const filterFlag = (options.enableFilters !== false) ? '0' : '1';

    // Try memfsPath first, fallback to filename only
    const swfArgv = memfsPath;

    const argvStrings = ['player', swfArgv, '0', renderer, filterFlag];
    const argc = argvStrings.length;

    console.log('[WAFlash-ReFlexed] Calling _main with:', argvStrings);

    // Allocate argv in WASM heap
    const argvPtr = M._malloc(argc * 4);
    const ptrs = argvStrings.map(s => writeString(M, s));
    ptrs.forEach((ptr, i) => M.HEAP32[(argvPtr >> 2) + i] = ptr);

    // Call _main()
    try {
        M._main(argc, argvPtr);
    } catch(e) {
        if (!String(e).includes('ExitStatus')) throw e;
    }

    // Cleanup argv
    ptrs.forEach(ptr => M._free(ptr));
    M._free(argvPtr);

    // Frame loop
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
        play()   { try { M._Play();  } catch(e) {} },
        stop()   { try { M._Stop();  } catch(e) {} },
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
