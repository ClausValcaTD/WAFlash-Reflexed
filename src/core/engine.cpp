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
#include <cstdlib>
#include <cstring>
#include <cstdio>

EngineContext* g_engine = nullptr;
waflash::AVM2Context* g_avm2 = nullptr;

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

    // 1. Execute AVM2 frame tick
    if (g_avm2) {
        g_avm2->executeFrame();
    }

    // 2. Advance MovieClip timelines
    g_engine->timeline_counter++;
}
