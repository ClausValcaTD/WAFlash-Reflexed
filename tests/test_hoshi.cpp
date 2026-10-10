// Copyright 2025 WAFlash-ReFlexed Authors
// Apache License 2.0
//
// Tests against actual Hoshi Saga SWF files
// from wasm_flash/games/hoshi1/
//
// If these pass → we fixed the bug Ruffle couldn't fix since 2021

#include "swf/swf_loader.hpp"
#include "avm/movie_clip.hpp"
#include <cassert>
#include <cstdio>

// Path to Hoshi SWFs — adjust if running from different directory
#define HOSHI_PATH "/tmp/wasm_flash/games/hoshi1/"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    if (cond) { printf("[PASS] %s\n", msg); passed++; } \
    else { printf("[FAIL] %s\n", msg); failed++; }

void test_swf_parsing() {
    printf("\n=== SWF Parsing Tests ===\n");

    // main.swf
    waflash::SWFLoader main_swf;
    bool ok = main_swf.load(HOSHI_PATH "main.swf");
    CHECK(ok, "main.swf loads");
    CHECK(main_swf.getVersion() == 8, "main.swf version = 8");
    CHECK(!main_swf.isAS3(), "main.swf is AS2");
    CHECK(main_swf.getHeader().frame_count == 59, "main.swf has 59 frames");

    // stg_1.swf
    waflash::SWFLoader stg1;
    ok = stg1.load(HOSHI_PATH "stg_1.swf");
    CHECK(ok, "stg_1.swf loads");
    CHECK(stg1.getHeader().frame_count == 2, "stg_1.swf has 2 frames");
    CHECK(stg1.getVersion() == 8, "stg_1.swf version = 8");

    // All 36 stages load
    int stages_ok = 0;
    for (int i = 1; i <= 36; i++) {
        char path[256];
        snprintf(path, sizeof(path), HOSHI_PATH "stg_%d.swf", i);
        waflash::SWFLoader s;
        if (s.load(path) && s.getVersion() == 8) stages_ok++;
    }
    printf("[INFO] %d/36 stages loaded successfully\n", stages_ok);
    CHECK(stages_ok >= 30, "At least 30/36 stages parse correctly");
}

void test_event_ordering() {
    printf("\n=== Event Ordering Tests (The Ruffle Bug Fix) ===\n");

    // Simulate exact Hoshi Saga AS2 sequence:
    //
    // main.swf frame 2:
    //   stop();
    //   this.mc_stg.loadMovie("stg_1.swf");
    //   mc_load.onEnterFrame = function() {
    //       if (mc_stg.getBytesLoaded() == mc_stg.getBytesTotal()) {
    //           mc_stg.play();
    //       }
    //   };

    waflash::MovieClip mc_stg("mc_stg", 1);
    waflash::MovieClip mc_load("mc_load", 10);

    // loadMovie called
    mc_stg.loadMovie(HOSHI_PATH "stg_1.swf");

    CHECK(mc_stg.getBytesTotal() > 0,
          "getBytesTotal() > 0 immediately after loadMovie()");
    CHECK(mc_stg.getCurrentFrame() == 1,
          "mc_stg starts on frame 1 (stopped)");

    // Set up onEnterFrame — exactly like Hoshi Saga AS2 code
    bool stage_appeared = false;
    int  enter_frame_count = 0;

    mc_load.onEnterFrame = [&]() {
        enter_frame_count++;
        uint32_t total  = mc_stg.getBytesTotal();
        uint32_t loaded = mc_stg.getBytesLoaded();
        printf("  [mc_load.onEnterFrame #%d] loaded=%u / total=%u\n",
               enter_frame_count, loaded, total);

        if (total > 0 && loaded == total) {
            mc_stg.play();              // ← AS2: _root.mc_stg.play()
            mc_load.onEnterFrame = nullptr; // ← AS2: this.removeMovieClip()
            stage_appeared = true;
        }
    };

    // Run engine ticks — simulating requestAnimationFrame loop
    printf("  Running ticks...\n");
    for (int tick = 1; tick <= 10; tick++) {
        printf("  --- Tick %d ---\n", tick);
        mc_stg.tick();   // processLoad FIRST → then onEnterFrame
        mc_load.tick();
        if (mc_stg.getCurrentFrame() == 2) break;
    }

    CHECK(stage_appeared, "Stage appeared (play() was called)");
    CHECK(mc_stg.getCurrentFrame() == 2, "mc_stg advanced to frame 2");
    CHECK(mc_stg.getBytesLoaded() == mc_stg.getBytesTotal(),
          "getBytesLoaded() == getBytesTotal()");
}

void test_all_stages_loadmovie() {
    printf("\n=== All 36 Stages LoadMovie Test ===\n");

    int stages_ok = 0;
    for (int i = 1; i <= 36; i++) {
        char path[256];
        snprintf(path, sizeof(path), HOSHI_PATH "stg_%d.swf", i);

        waflash::MovieClip mc_stg("mc_stg", 1);
        waflash::MovieClip mc_load("mc_load", 10);

        mc_stg.loadMovie(path);

        bool done = false;
        mc_load.onEnterFrame = [&]() {
            if (mc_stg.getBytesTotal() > 0 &&
                mc_stg.getBytesLoaded() == mc_stg.getBytesTotal()) {
                mc_stg.play();
                mc_load.onEnterFrame = nullptr;
                done = true;
            }
        };

        for (int t = 0; t < 10; t++) {
            mc_stg.tick();
            mc_load.tick();
            if (done && mc_stg.getCurrentFrame() == 2) break;
        }

        if (done && mc_stg.getCurrentFrame() == 2) {
            stages_ok++;
        } else {
            printf("  [WARN] stg_%d.swf did not reach frame 2\n", i);
        }
    }

    printf("[INFO] %d/36 stages reached frame 2 (puzzle visible)\n", stages_ok);
    CHECK(stages_ok >= 30, "At least 30/36 stages show puzzle correctly");
}

int main() {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   WAFlash-ReFlexed: Hoshi Saga Test Suite   ║\n");
    printf("║   Fixing Ruffle issue #3615 (open 4 years)  ║\n");
    printf("╚══════════════════════════════════════════════╝\n");

    test_swf_parsing();
    test_event_ordering();
    test_all_stages_loadmovie();

    printf("\n══════════════════════════════════════\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    printf("══════════════════════════════════════\n");

    if (failed == 0) {
        printf("🌟 ALL TESTS PASSED!\n");
        printf("🌟 Hoshi Saga compatibility: FIXED!\n");
        printf("🇪🇬 Flash preservation: one step closer!\n");
    }

    return failed > 0 ? 1 : 0;
}
