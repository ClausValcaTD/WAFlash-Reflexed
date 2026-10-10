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

#include "engine.hpp"
#include "avm/avm2.hpp"
#include "avm/movie_clip.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>
#include <memory>

EngineContext* g_engine = nullptr;
waflash::AVM2Context* g_avm2 = nullptr;
static std::vector<std::unique_ptr<waflash::MovieClip>> g_clips;

void engine_init(const char* swf_url, bool webgl, bool disable_filters) {
    (void)swf_url;
    (void)disable_filters;

    if (!g_engine) {
        g_engine = static_cast<EngineContext*>(std::calloc(1, sizeof(EngineContext)));
    }

    if (g_engine) {
        g_engine->timeline_counter = 1L;
        g_engine->frame_state = 0L;
        g_engine->float_constants = 1.0;
        g_engine->playback_state = STATE_PLAYING; // 5
        g_engine->init_flag = 1;
        g_engine->audio_state = 0;
        g_engine->render_state = webgl ? 1 : 0;
        g_engine->telemetry_flag = 0;
        g_engine->reserved = 0;
    }

    if (!g_avm2) {
        g_avm2 = new waflash::AVM2Context();
        g_avm2->init();
        if (swf_url) {
            g_avm2->loadSWF(swf_url);
        }
    }
}

EngineContext* get_engine_context() {
    return g_engine;
}

extern "C" void engine_tick() {
    if (!g_engine) return;
    if (g_engine->playback_state != STATE_PLAYING) return;

    // Tick all clips — each does processLoad → advanceFrame → onEnterFrame
    for (auto& clip : g_clips) {
        if (clip) clip->tick();
    }

    if (g_avm2) g_avm2->executeFrame();
    g_engine->timeline_counter++;
}


// Background color and SWF dimensions exported to JS/WASM
static uint8_t g_bg_r = 255, g_bg_g = 255, g_bg_b = 255;
static int g_frame_ready = 0;
static int g_swf_width = 550;
static int g_swf_height = 400;

extern "C" void setBackgroundColor(uint8_t r, uint8_t g, uint8_t b) {
    g_bg_r = r; g_bg_g = g; g_bg_b = b;
    std::printf("[Engine] Background color: rgb(%d,%d,%d)\n", r, g, b);
}

extern "C" uint32_t getBackgroundColor() {
    return ((uint32_t)g_bg_r << 16) |
           ((uint32_t)g_bg_g << 8)  |
           ((uint32_t)g_bg_b);
}

extern "C" int isFrameReady() {
    int r = g_frame_ready;
    g_frame_ready = 0;
    return r;
}

extern "C" void signalFrameReady() {
    g_frame_ready = 1;
}

extern "C" void setSWFDimensions(int w, int h) {
    g_swf_width = w;
    g_swf_height = h;
    std::printf("[Engine] SWF Dimensions set: %dx%d\n", w, h);
}

extern "C" int getSWFWidth() {
    return g_swf_width;
}

extern "C" int getSWFHeight() {
    return g_swf_height;
}
