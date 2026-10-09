# TASK: WAFlash-ReFlexed — Open Source Flash Player Engine

## Mission
Build a clean-room, open-source Flash Player engine for web game preservation.
Target audience: Internet Archive, Flashpoint Archive, and the Flash nostalgia community.
License: Apache 2.0

---

## Context
Read `docs/architecture_spec.md` carefully before writing any code.
Read `docs/binary_analysis.md` and `docs/deep_analysis.md` as reference.

This is a **clean-room implementation** — implement behavior from the spec only.
Do NOT copy any Adobe, WAFlash, or proprietary binary code.

---

## What to Build

### Step 1: Project Structure
Create the following layout:

```
src/
├── main.cpp                 — Entry point (4-param schema from spec)
├── core/
│   ├── engine.hpp           — EngineContext struct (1,120 bytes)
│   ├── engine.cpp           — Core init, state machine
│   └── memfs.cpp            — Virtual filesystem (/waflashso)
├── avm/
│   ├── avm2.hpp             — AVM2 interface (wraps avmplus)
│   └── avm2.cpp             — ActionScript VM integration
├── render/
│   ├── renderer.hpp         — Renderer interface
│   ├── webgl_renderer.cpp   — WebGL backend ("webgl" mode)
│   └── canvas_renderer.cpp  — 2D Canvas fallback ("default" mode)
├── audio/
│   ├── audio.hpp            — Audio interface
│   └── audio.cpp            — reopenBuffer stub + WebAudio hooks
├── bridge/
│   ├── external_interface.hpp
│   └── external_interface.cpp — AS→JS ExternalInterface dispatcher
└── memory/
    └── allocator.cpp        — dlmalloc integration

deps/
├── avmplus/                 — git submodule: adobe-flash/avmplus
└── swf-parser/              — git submodule: open-flash/swf-parser

CMakeLists.txt               — CMake build system
Makefile.emscripten          — Emscripten/WASM build
```

---

## Step 2: Core Engine (`src/core/engine.hpp`)

Implement the `EngineContext` struct matching the spec:

```cpp
struct EngineContext {
    void*    vtable;          // offset 0
    int      subsystem_flags; // offset 1
    // ... padding ...
    long     timeline_counter;// offset 129
    long     frame_state;     // offset 130
    double   float_constants; // offset 135 (IEEE 754)
    // ... padding ...
    int      playback_state;  // offset 258  ← STATE_PLAYING=5, STATE_PAUSED=6
    int      init_flag;       // offset 272
    short    audio_state;     // offset 546
    short    render_state;    // offset 552
    // ... padding to 1,120 bytes total
    uint8_t  telemetry_flag;  // offset 1093
    uint8_t  reserved;        // offset 1106
};

#define STATE_PLAYING 5
#define STATE_PAUSED  6
```

---

## Step 3: Entry Point (`src/main.cpp`)

Implement `main()` matching the 4-parameter schema from `docs/architecture_spec.md`:

```cpp
int main(int argc, char** argv) {
    // argv[0] = executable name
    // argv[1] = swfUrl        (max 4095 bytes)
    // argv[2] = subsystem     ("0" = standard)
    // argv[3] = renderer      ("webgl" or "default")
    // argv[4] = disableFilters("0" = on, "1" = off)
}
```

Rules:
- SWF URL buffer: exactly 4,095 bytes max (stack allocated)
- Renderer: strcmp against "webgl"
- Filter toggle: check argv[4][0] == '1' (ASCII 49)
- Allocate EngineContext with calloc(1, 1120)
- Store engine pointer in global
- Mount MEMFS at "/waflashso" for SharedObjects
- Log startup banner: `"WAFlash-ReFlexed v0.1.0 (%s)\n"`

---

## Step 4: Exported Functions

Implement these 7 exported functions exactly:

```cpp
// 1. Resume timeline
extern "C" void Play() {
    if (g_engine) g_engine->playback_state = STATE_PLAYING;
}

// 2. Pause timeline
extern "C" void Stop() {
    if (g_engine) g_engine->playback_state = STATE_PAUSED;
}

// 3. Audio resync stub
extern "C" int reopenBuffer(int a, int b, int c) {
    return 0; // host JS handles real audio work
}

// 4. AS→JS bridge
extern "C" int invokeExternalCallback(void* funcName, void* args);

// 5-7: malloc, free, strlen (standard C — use system implementations)
```

---

## Step 5: Memory Management

Integrate `dlmalloc` with these specs from `docs/architecture_spec.md`:

- Smallbins: chunks ≤ 244 bytes → O(1) bitmask lookup
- Treebins: chunks > 244 bytes → best-fit tree
- Chunk header: 8-byte prefix, low bits = flags (PREV_INUSE, IS_MMAPPED)
- WebGL buffers: use `memalign(16, size)` for SIMD alignment
- Max heap: 2GB

---

## Step 6: CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(waflash-reflexed VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# avmplus AVM2 dependency
add_subdirectory(deps/avmplus)

# Main engine
add_executable(waflash-reflexed
    src/main.cpp
    src/core/engine.cpp
    src/core/memfs.cpp
    src/avm/avm2.cpp
    src/render/webgl_renderer.cpp
    src/render/canvas_renderer.cpp
    src/audio/audio.cpp
    src/bridge/external_interface.cpp
    src/memory/allocator.cpp
)

target_include_directories(waflash-reflexed PRIVATE
    src/
    deps/avmplus/core/
    deps/swf-parser/
)

target_link_libraries(waflash-reflexed avmplus)
```

---

## Step 7: Emscripten Build (`Makefile.emscripten`)

```makefile
EMCC = emcc
CXXFLAGS = -O2 -std=c++17
EXPORTED_FUNCTIONS = '["_main","_Play","_Stop","_reopenBuffer","_invokeExternalCallback","_malloc","_free","_strlen"]'
EXPORTED_RUNTIME = '["ccall","cwrap","FS"]'

WASM_FLAGS = \
    -s WASM=1 \
    -s INITIAL_MEMORY=16777216 \
    -s MAXIMUM_MEMORY=2147483648 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_FUNCTIONS=$(EXPORTED_FUNCTIONS) \
    -s EXPORTED_RUNTIME_METHODS=$(EXPORTED_RUNTIME) \
    -s USE_WEBGL2=1

all:
	$(EMCC) $(CXXFLAGS) $(WASM_FLAGS) \
	    src/main.cpp src/core/engine.cpp \
	    src/avm/avm2.cpp src/render/webgl_renderer.cpp \
	    src/audio/audio.cpp src/bridge/external_interface.cpp \
	    -Ideps/avmplus/core \
	    -o dist/waflash-reflexed.js
```

---

## Step 8: JS Loader (`dist/waflash-reflexed-loader.js`)

Create a minimal JS loader matching the WAFlash embedding API:

```javascript
async function createWaflashReflexed(swfUrl, options = {}) {
    const wasmModule = await WebAssembly.instantiateStreaming(
        fetch('waflash-reflexed.wasm'), importObject
    );
    
    const { _main, _Play, _Stop, memory } = wasmModule.instance.exports;
    
    // Call main with 4-param schema
    // argv = [executable, swfUrl, "0", renderer, filterFlag]
    const renderer = options.gpu ? "webgl" : "default";
    const filters  = options.enableFilters ? "0" : "1";
    
    // ... write strings to linear memory ...
    // ... call _main(5, argvPtr) ...
    
    return {
        play:  () => _Play(),
        stop:  () => _Stop(),
        memory: memory
    };
}
```

---

## Compatibility Targets (Priority Order)

Test against these SWF files first:

1. **Hoshi Saga** (Nekogames) — 36 stages, each with unique AS2 mechanic
2. **Hoshi Saga Ringo** — Listed in "250 Indie Games You Must Play"
3. **Cursor\*10** (Nekogames) — Self-co-op timeline recording mechanic
4. Simple AS2 animations (smoke test)
5. AS3 content (stretch goal)

---

## What NOT to Do

- Do NOT copy any code from WAFlash, Adobe Flash Player, or any proprietary binary
- Do NOT implement DRM (DRMManager, DRMContentData) — preservation only
- Do NOT add telemetry or analytics
- Do NOT add any tracking (no Google Analytics UA codes)
- Keep it simple — target AS2 games first, AS3 second

---

## Success Criteria

- [ ] Hoshi Saga stage 1 renders correctly
- [ ] Play/Stop state machine works
- [ ] SWF URL loading works
- [ ] WebGL renderer initializes
- [ ] Audio stub compiles without errors
- [ ] Builds to WASM via Emscripten
- [ ] Builds natively via CMake

---

*Apache License 2.0 — Flash preservation. Free forever. 🇪🇬*
