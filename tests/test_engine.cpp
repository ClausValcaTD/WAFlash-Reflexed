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
#include <vector>
#include <zlib.h>

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

    // Test SWF loading - FWS (uncompressed)
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
    assert(loader.isCompressed() == false);

    // Test CWS (zlib compressed) SWF
    // Construct uncompressed body: RECT + FrameRate (12.0) + FrameCount (1)
    uint8_t body_raw[12] = { 0x78, 0, 0x05, 0x00, 0x00, 0x0c, 0x01, 0x00, 0, 0, 0, 0 };
    uLongf dest_len = 100;
    std::vector<uint8_t> comp_body(dest_len);
    int z_res = compress(comp_body.data(), &dest_len, body_raw, sizeof(body_raw));
    assert(z_res == Z_OK);
    comp_body.resize(dest_len);

    std::vector<uint8_t> cws_swf;
    uint32_t file_len = 8 + sizeof(body_raw);
    cws_swf.push_back('C');
    cws_swf.push_back('W');
    cws_swf.push_back('S');
    cws_swf.push_back(8); // Flash 8
    cws_swf.push_back(file_len & 0xFF);
    cws_swf.push_back((file_len >> 8) & 0xFF);
    cws_swf.push_back((file_len >> 16) & 0xFF);
    cws_swf.push_back((file_len >> 24) & 0xFF);
    cws_swf.insert(cws_swf.end(), comp_body.begin(), comp_body.end());

    waflash::SWFLoader cws_loader;
    bool cws_loaded = cws_loader.loadFromMemory(cws_swf.data(), cws_swf.size());
    assert(cws_loaded == true);
    assert(cws_loader.isCompressed() == true);
    assert(cws_loader.getVersion() == 8);
    assert(cws_loader.isAS3() == false); // version 8 = AS2
    assert(cws_loader.getDecompressedBody().size() == sizeof(body_raw));
    printf("[Test] CWS decompression: %s\n",
        cws_loader.isCompressed() ? "compressed" : "uncompressed");

    // Test version detection
    waflash::SWFLoader as2_loader;
    uint8_t as2_swf[12] = {'F','W','S', 8, 12,0,0,0, 0x78,0, 0x01,0x00};
    as2_loader.loadFromMemory(as2_swf, 12);
    assert(as2_loader.isAS3() == false);  // version 8 = AS2
    assert(as2_loader.getVersion() == 8);

    // Test avmplus init
    waflash::AVM2Context avm;
    assert(avm.init() == true);
#ifdef HAS_AVMPLUS
    assert(avm.getAvmCore() != nullptr);
#endif

    // Test frame execution ticker
    int64_t prev_counter = ctx->timeline_counter;
    engine_tick();
    assert(ctx->timeline_counter == prev_counter + 1);

    printf("All tests passed successfully!\n");
    return 0;
}
