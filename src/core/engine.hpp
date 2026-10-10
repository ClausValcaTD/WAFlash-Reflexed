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

#ifndef WAFLASH_CORE_ENGINE_HPP
#define WAFLASH_CORE_ENGINE_HPP

#include <cstdint>
#include <cstddef>

#define STATE_PLAYING 5
#define STATE_PAUSED  6

#pragma pack(push, 1)
struct EngineContext {
    uint64_t vtable;            // offset 0
    int32_t subsystem_flags;    // offset 8
    uint8_t pad0[120];          // offset 12 -> 132
    int64_t timeline_counter;   // offset 132
    int64_t frame_state;        // offset 140
    double float_constants;     // offset 148
    uint8_t pad1[102];          // offset 156 -> 258
    int32_t playback_state;     // offset 258 (STATE_PLAYING=5, STATE_PAUSED=6)
    int32_t init_flag;          // offset 262
    uint8_t pad2[280];          // offset 266 -> 546
    int16_t audio_state;        // offset 546
    int16_t render_state;       // offset 548
    uint8_t pad3[543];          // offset 550 -> 1093
    uint8_t telemetry_flag;     // offset 1093
    uint8_t pad4[12];           // offset 1094 -> 1106
    uint8_t reserved;           // offset 1106
    uint8_t pad5[13];           // offset 1107 -> 1120
};
#pragma pack(pop)

static_assert(sizeof(EngineContext) == 1120, "EngineContext must be exactly 1,120 bytes");
static_assert(offsetof(EngineContext, playback_state) == 258, "playback_state must be at byte offset 258");
static_assert(offsetof(EngineContext, telemetry_flag) == 1093, "telemetry_flag must be at byte offset 1093");
static_assert(offsetof(EngineContext, reserved) == 1106, "reserved must be at byte offset 1106");

extern EngineContext* g_engine;

namespace waflash {
    class AVM2Context;
}

extern waflash::AVM2Context* g_avm2;

void engine_init(const char* swf_url, bool webgl, bool disable_filters);
EngineContext* get_engine_context();

extern "C" void engine_tick();

#endif // WAFLASH_CORE_ENGINE_HPP

extern "C" void setBackgroundColor(uint8_t r, uint8_t g, uint8_t b);
extern "C" uint32_t getBackgroundColor();
extern "C" int isFrameReady();
extern "C" void signalFrameReady();
extern "C" void setSWFDimensions(int w, int h);
extern "C" int getSWFWidth();
extern "C" int getSWFHeight();
