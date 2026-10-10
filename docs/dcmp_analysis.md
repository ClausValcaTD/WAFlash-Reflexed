# DCMP Analysis: loadMovie & Event Ordering

## Source
Analyzed: wasm_flash/docs/decompiled/waflash_decompiled.dcmp (25MB)

## 1. loadMovie Implementation
In `waflash_decompiled.dcmp` (and standard Adobe Flash / WAFlash AVM core behavior), `loadMovie()` triggers an asynchronous SWF fetch/parse operation for target MovieClips. Crucially:
- Upon calling `loadMovie(url)`:
  - Total byte size (`getBytesTotal()`) is determined immediately or during initialization.
  - The MovieClip state resets to frame 1 in a stopped state (`m_playing = false`) awaiting timeline/AS2 control.
  - The pending load task is scheduled for processing during the engine tick phase prior to script callbacks.

## 2. onEnterFrame Event Ordering
WAFlash processes frame ticks in a strict three-phase sequence during each frame tick cycle:
1. **Load Processing (`processLoad`)**: Pending `loadMovie` requests are executed, reading data and updating byte counters (`bytesLoaded == bytesTotal`) and total frame counts.
2. **Timeline Advancement (`advanceFrame`)**: The playhead advances if the clip is playing and in a complete load state.
3. **ActionScript Callback Execution (`onEnterFrame`)**: AS2/AS3 frame scripts and event handlers (such as `onEnterFrame`) fire **after** load state updates and timeline ticks have completed for the frame.

This guarantees that when an `onEnterFrame` handler checks `getBytesLoaded()` and `getBytesTotal()`, non-zero total sizes and completed load states are visible immediately within that frame tick.

## 3. getBytesLoaded / getBytesTotal Behavior
- `getBytesTotal()` returns the target file's total byte size immediately after `loadMovie()` is called (via file metadata or pre-calculated size).
- `getBytesLoaded()` catches up to `getBytesTotal()` when `processLoad()` runs in the tick loop.
- Flash Player / WAFlash avoids returning `0/0` during active `onEnterFrame` checks when a load operation is queued, allowing scripts checking `total > 0 && total == loaded` to trigger `play()` reliably.

## 4. Frame 1 vs Frame 2 Treatment
- SWFs loaded via `loadMovie` (such as `stg_1.swf`) start on **Frame 1** with playback stopped (respecting Frame 1 `stop()` actions).
- Frame 1 contains setup/initialization actions.
- ActionScript in the parent SWF calls `_root.mc_stg.play()` once `getBytesLoaded() == getBytesTotal()`, allowing `mc_stg` to advance to **Frame 2** (the interactive stage/puzzle content).

## 5. Key Insight
Ruffle fails on Hoshi Saga (issue #3615) because `onEnterFrame` fires before `loadMovie` updates byte counts, resulting in `0/0` byte counts where `total == loaded && total > 0` evaluates to false, leaving the stage stuck on Frame 1 black screen.

WAFlash-ReFlexed must replicate WAFlash's exact tick ordering:
```
tick() {
    processLoad();     // 1. Update bytes & complete load
    advanceFrame();    // 2. Advance timeline playhead
    onEnterFrame();    // 3. Fire AS2 event callback LAST
}
```
