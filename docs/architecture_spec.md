# WAFlash-ReFlexed: Architecture Specification
> Clean-room Flash Player engine for preservation purposes.
> Derived from binary behavioral analysis — no Adobe source code copied.

---

## 1. Project Overview

**Goal:** Open-source Flash Player runtime targeting AS2/AS3 SWF files,
with focus on complex puzzle games (e.g. Hoshi Saga series by Nekogames)
that existing emulators (Ruffle) fail to run correctly.

**License:** Apache 2.0  
**Target:** WebAssembly (via Emscripten) + Native C++  
**Dependencies:**
- `adobe-flash/avmplus` — AVM2 ActionScript VM (GPL/LGPL/MPL tri-license)
- `open-flash/swf-parser` — SWF binary parser (MIT)
- Lightspark rendering pipeline reference (LGPL v3)
- dlmalloc — Dynamic memory allocator (Public Domain)

---

## 2. Entry Point Schema

The core engine exposes a standard C `main()` entry point with a strict
4-parameter argument schema:

```c
int main(int argc, char** argv);
```

### argv Layout in Linear Memory

| Index | Parameter | Values | Description |
|:------|:----------|:-------|:------------|
| argv[0] | executable | `"player"` | Executable name |
| argv[1] | swfUrl | `"https://..."` or local path | SWF file URL (max 4095 bytes) |
| argv[2] | subsystem | `"0"` | Subsystem mode flag (default: standard SWF) |
| argv[3] | renderer | `"webgl"` or `"default"` | Graphics backend selector |
| argv[4] | disableFilters | `"0"` or `"1"` | `"0"` = filters ON, `"1"` = filters OFF |

### Parameter Handling Rules
- SWF URL is copied into a stack buffer capped at **4,095 bytes**
- Renderer is selected by string comparison against literal `"webgl"`
- Filter toggle is detected by checking `argv[4][0] == '1'` (ASCII 49)
- Engine version string: `"2.8.27"` (reference target)
- Virtual SharedObject storage mounts at path `"/waflashso"`

---

## 3. Engine State Machine

### Global Instance Pointer
The entire engine state is managed through a **single global pointer**
stored at a fixed memory address. All subsystems reference this pointer.

```c
// Pseudo-structure (1,120 bytes allocated via calloc)
struct EngineContext {
    // offset 0:   vtable pointer
    // offset 1:   subsystem flags
    // offset 129: timeline counters (long)
    // offset 130: frame state (long)  
    // offset 135: float constants (IEEE 754)
    // offset 258: PLAYBACK STATE FLAG  ← critical
    // offset 272: initialization flag
    // offset 546: audio state (short)
    // offset 552: render state (short)
    // offset 1093: telemetry flag (byte)
    // offset 1106: reserved (byte)
};
```

### Playback State Flags (offset 258)

| Value | State | Description |
|:------|:------|:------------|
| `5` | `STATE_PLAYING` | Timeline active, AS bytecode executing, events dispatching |
| `6` | `STATE_PAUSED` | Timeline suspended, rendering buffers intact |

### Exported Control Functions

```c
// Resume SWF timeline
void Play() {
    EngineContext* ctx = global_engine_ptr;
    if (ctx) ctx->state_flag = 5; // STATE_PLAYING
}

// Pause SWF timeline  
void Stop() {
    EngineContext* ctx = global_engine_ptr;
    if (ctx) ctx->state_flag = 6; // STATE_PAUSED
}
```

---

## 4. Memory Architecture

### Linear Memory Layout

```
+------------------+------------------+------------------+-------------------+
| Static Data      | Stack Space      | Dynamic Heap     | Growth Region     |
| Segments (0-1MB) | (stack pointer)  | (dlmalloc)       | Up to 2GB         |
+------------------+------------------+------------------+-------------------+
0                  ~1,005,488         ~1,080,908         32,768 pages max
```

### dlmalloc Implementation

The engine embeds **Doug Lea's dlmalloc** (ptmalloc variant):

```c
// Chunk header structure (8-byte prefix at ptr - 8)
struct ChunkHeader {
    size_t prev_size;  // size of previous chunk (if free)
    size_t size;       // size of this chunk + flags in low bits
    // bit 0 = PREV_INUSE
    // bit 1 = IS_MMAPPED
};
```

**Smallbins** (≤ 244 bytes):
- Indexed at base offset `1,080,948`
- Bitmask at offset `1,080,908`
- O(1) allocation fast-path

**Treebins** (> 244 bytes):
- Root array at offset `1,081,212`
- Best-fit tree traversal

**Memory Growth:**
- Top chunk pointer at offset `1,080,932`
- Expands via `memory.grow` up to **2GB** (32,768 × 64KB pages)

### Subsystem Heap Segregation

| Subsystem | Allocator | Alignment |
|:----------|:----------|:----------|
| WebGL Texture Buffers | `memalign` | 16-byte (SIMD) |
| ActionScript GC Heap | `malloc` | standard |
| Audio Ring Buffers | `malloc` | standard |
| Engine Context | `calloc(1, 1120)` | standard |

---

## 5. Subsystems

### 5.1 ActionScript Virtual Machine (AVM2)

**Source:** `adobe-flash/avmplus` (GPL/LGPL/MPL)

Key AVM2 internals confirmed present:
- `avmplus::NativeID` — native class registry
- `flash.display.MovieClip` / `Sprite` / `Stage`
- `flash.events.Event` dispatcher
- `flash.external.ExternalInterface`
- `flash.media.Camera` / `Sound`
- `flash.text.engine.TextBlock`
- `NetStream` buffer management

### 5.2 Rendering Pipeline

**Backend selector:** `argv[3]`

```
"webgl"   → WebGL hardware-accelerated path
              glCreateShader, glBindTexture,
              glDrawElements, framebuffers
              
"default" → 2D Canvas software blitting path
```

**Filter system** (toggled by `argv[4]`):
- BlurFilter
- DropShadowFilter
- DisplayObject bitmap filter passes

### 5.3 Audio Subsystem

**Architecture:** Host-managed WebAudio + WASM stub

```c
// WASM stub — returns immediately
// Real work done by host JS WebAudio bindings
int reopenBuffer(int a, int b, int c) {
    return 0; // success
}
```

Host JS handles:
- `ScriptProcessorNode` streaming
- `AudioContext` sample rate conversion
- 3D audio positioning (`coneInnerAngle`, `rolloffFactor`)
- Buffer flushing on tab focus change

### 5.4 ExternalInterface Bridge

ActionScript → JavaScript callback dispatcher:

```c
// Simplified protocol
int invokeExternalCallback(ActionScriptValue* funcName,
                            ActionScriptValue* args) {
    // 1. Unpack AS values from WASM linear memory
    // 2. Lookup function in "_callbacks" registry via f_tx()
    // 3. Dispatch to host JS via "call" method
    // 4. Marshal return value back to AVM context
    return result;
}
```

String lookup key: `"_callbacks"`  
Dispatch method: `"call"`

### 5.5 Virtual File System (MEMFS)

- Mount point: `"/waflashso"`
- Purpose: Flash `SharedObject` persistence (LSO files)
- Operations: `FS_createDataFile`, `FS_createPreloadedFile`,
  `open`, `read`, `write`, `close`
- SWF assets fetched async and stored in MEMFS

---

## 6. Build Target

### Emscripten / WASM

```cmake
# Target compilation flags (reference)
emcc -O2 \
     -s WASM=1 \
     -s INITIAL_MEMORY=16777216 \
     -s MAXIMUM_MEMORY=2147483648 \
     -s ALLOW_MEMORY_GROWTH=1 \
     -s EXPORTED_FUNCTIONS='["_main","_Play","_Stop","_reopenBuffer","_invokeExternalCallback","_malloc","_free"]' \
     -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
     -o waflash-reflexed.js \
     src/main.cpp
```

### Native C++ (Libretro core target)

```cpp
// Frame driver loop
int argc = 5;
const char* argv[] = {
    "player",
    swf_url,
    "0",
    "webgl",   // or "default"
    "0"        // filters enabled
};

_main(argc, (char**)argv);

// Game loop
while (running) {
    EngineContext* ctx = get_engine_context();
    if (ctx && ctx->state_flag == 5) { // STATE_PLAYING
        render_frame();
        pump_audio();
    }
}
```

---

## 7. Exported Symbol Reference

| Minified | Native Symbol | Kind | Purpose |
|:---------|:-------------|:-----|:--------|
| `xi` | `_main` | Function | Core init + SWF loader |
| `ri` | `_Play` | Function | Resume timeline |
| `si` | `_Stop` | Function | Pause timeline |
| `ti` | `_reopenBuffer` | Function | Audio resync stub |
| `ui` | `_invokeExternalCallback` | Function | AS→JS bridge |
| `zi` | `_malloc` | Function | Heap allocator |
| `wi` | `_free` | Function | Heap deallocator |
| `vi` | `_strlen` | Function | C-string length |
| `Ai` | `___errno_location` | Function | POSIX errno |
| `Bi` | `_emscripten_builtin_memalign` | Function | Aligned alloc |
| `pi` | `memory` | Memory | Linear memory (WebAssembly.Memory) |
| `yi` | `table` | Table | Indirect call table (WebAssembly.Table) |

---

## 8. Known Compatibility Targets

Games confirmed broken in Ruffle, target for this engine:

| Game | Developer | Issue |
|:-----|:----------|:------|
| Hoshi Saga (all series) | Nekogames / Yoshio Ishii | Complex AS2 per-stage mechanics |
| Hoshi Saga Ringo | Nekogames | Multi-mechanic puzzle engine |
| Cursor*10 | Nekogames | Self-co-op timeline recording |

---

*This specification is derived entirely from behavioral binary analysis.*  
*No Adobe source code was copied or reproduced.*  
*Apache License 2.0 — Free forever. 🇪🇬*
