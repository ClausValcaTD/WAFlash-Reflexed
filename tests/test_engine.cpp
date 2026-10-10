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

#include "core/engine.hpp"
#include "audio/audio.hpp"
#include "swf/swf_loader.hpp"
#include "avm/avm2.hpp"
#include <cassert>
#include <cstdio>
#include <cstddef>

extern "C" void Play();
extern "C" void Stop();

int main() {
    printf("Running unit tests...\n");

    assert(sizeof(EngineContext) == 1120);
    assert(offsetof(EngineContext, playback_state) == 258);

    engine_init("https://example.com/hoshi_saga.swf", true, false);

    EngineContext* ctx = get_engine_context();
    assert(ctx != nullptr);
    assert(ctx->playback_state == STATE_PLAYING);

    Stop();
    assert(ctx->playback_state == STATE_PAUSED);

    Play();
    assert(ctx->playback_state == STATE_PLAYING);

    int audio_res = reopenBuffer(1, 2, 3);
    assert(audio_res == 0);

    // Test SWF loading
    uint8_t minimal_swf_data[12] = {
        'F', 'W', 'S', 10,   // FWS signature, version 10 (AS3)
        12, 0, 0, 0,         // File length 12
        0x78, 0, 0x05, 0x00  // Rect & frame info
    };

    waflash::SWFLoader loader;
    bool loaded = loader.loadFromMemory(minimal_swf_data, sizeof(minimal_swf_data));
    assert(loaded == true);
    assert(loader.getVersion() == 10);
    assert(loader.isAS3() == true);

    // Test AVM2 init
    waflash::AVM2Context avm;
    assert(avm.init() == true);

    // Test frame execution ticker
    int64_t prev_counter = ctx->timeline_counter;
    engine_tick();
    assert(ctx->timeline_counter == prev_counter + 1);

    printf("All tests passed successfully!\n");
    return 0;
}
