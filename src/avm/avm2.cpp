#include "core/engine.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
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

#include "avm2.hpp"
#include "../swf/swf_tags.hpp"
#include "../swf/swf_loader.hpp"
#include "../render/display_list.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>

#ifdef HAS_AVMPLUS
#include "MMgc/GC.h"
#include "MMgc/GCHeap.h"
#include "AvmCore.h"
#include "Toplevel.h"

// Minimal AvmCore subclass for WAFlash
class WaflashAvmCore : public avmplus::AvmCore {
public:
    WaflashAvmCore(MMgc::GC* gc) : avmplus::AvmCore(gc, avmplus::kApiVersion_AIR_1_51) {}

    void interrupt(avmplus::Toplevel*, avmplus::InterruptReason) override {}
    void stackOverflow(avmplus::Toplevel*) override {}

    avmplus::String* readFileForEval(
        avmplus::String* /*referencingFile*/,
        avmplus::String* /*filename*/) override { return nullptr; }
};
#endif

namespace waflash {

// Store parsed SWF data
static std::vector<uint8_t>  g_swf_body;
static SWFTagParser          g_tag_parser;
static DisplayList           g_display_list;
static int                   g_current_frame = 0;
static int                   g_frame_count   = 0;

// Store tags per frame for playback
struct FrameData {
    std::vector<SWFTag> tags;
    std::vector<std::vector<uint8_t>> tag_data; // owned copies
};
static std::vector<FrameData> g_frames;

AVM2Context::AVM2Context()
    : m_initialized(false), m_gc(nullptr), m_core(nullptr), m_toplevel(nullptr) {}

AVM2Context::~AVM2Context() {
    shutdown();
}

bool AVM2Context::init() {
    if (m_initialized) return true;

#ifdef HAS_AVMPLUS
    // Initialize MMgc garbage collector
    MMgc::GCHeap::Init();
    MMgc::GCHeapConfig config;
    MMgc::GCHeap* heap = MMgc::GCHeap::GetGCHeap();
    m_gc = new MMgc::GC(heap, MMgc::GC::kIncrementalGC);

    // Initialize AvmCore
    m_core = new WaflashAvmCore(m_gc);
    m_initialized = (m_core != nullptr);

    std::printf("[AVM2] avmplus AvmCore initialized (real VM)\n");
#else
    m_initialized = true;
    std::printf("[AVM2] AVM2 stub initialized (no avmplus)\n");
#endif

    return m_initialized;
}

bool AVM2Context::loadSWF(const std::string& url) {
    if (!m_initialized) return false;

    std::printf("[AVM2] Loading SWF: %s\n", url.c_str());

    SWFLoader loader;
    if (!loader.load(url)) {
        std::printf("[AVM2] Failed to load SWF: %s\n", url.c_str());
        return false;
    }

    g_swf_body = loader.getDecompressedBody();
    if (g_swf_body.empty()) {
        std::printf("[AVM2] Empty SWF body\n");
        return false;
    }

    std::printf("[AVM2] SWF loaded: %zu bytes decompressed\n", g_swf_body.size());

    g_frames.clear();
    g_frames.emplace_back(); // frame 0

    g_tag_parser.parse(g_swf_body.data(), g_swf_body.size(),
        [](const SWFTag& tag) {
            FrameData& frame = g_frames.back();

            std::vector<uint8_t> data_copy(tag.data, tag.data + tag.length);
            frame.tag_data.push_back(std::move(data_copy));

            SWFTag tag_copy = tag;
            tag_copy.data = frame.tag_data.back().data();
            frame.tags.push_back(tag_copy);

            if (tag.id == TagID::ShowFrame) {
                g_frames.emplace_back();
            }
        });

    g_frame_count = static_cast<int>(g_frames.size());
    g_current_frame = 0;

    std::printf("[AVM2] Parsed %d frames, %d tag groups\n",
                g_frame_count, static_cast<int>(g_frames.size()));

    return true;
}

void AVM2Context::executeFrame() {
    if (!m_initialized) return;
    if (g_frames.empty()) return;
    if (g_current_frame >= g_frame_count) return;

    const FrameData& frame = g_frames[g_current_frame];

    for (const SWFTag& tag : frame.tags) {
        switch (tag.id) {
            case TagID::SetBackgroundColor:
                if (tag.length >= 3) {
                    setBackgroundColor(tag.data[0], tag.data[1], tag.data[2]);
                }
                break;

            case TagID::PlaceObject2: {
                if (tag.length < 3) break;
                uint8_t flags    = tag.data[0];
                uint16_t depth   = tag.data[1] | (tag.data[2] << 8);
                uint16_t char_id = 0;
                size_t pos = 3;

                bool has_char   = flags & 0x02;
                bool has_matrix = flags & 0x04;
                bool has_name   = flags & 0x20;
                (void)has_matrix;

                if (has_char && pos + 2 <= tag.length) {
                    char_id = tag.data[pos] | (tag.data[pos+1] << 8);
                    pos += 2;
                }

                std::string name = "";
                if (has_name) {
                    while (pos < tag.length && tag.data[pos]) {
                        name += static_cast<char>(tag.data[pos++]);
                    }
                }

                g_display_list.placeObject(depth, char_id, name);
                std::printf("[AVM2] PlaceObject2: depth=%d char=%d name='%s'\n",
                            depth, char_id, name.c_str());
#ifdef __EMSCRIPTEN__
                EM_ASM(({
                    var depth = $0;
                    var charId = $1;
                    var strName = UTF8ToString($2);

                    var canvas = document.getElementById("canvas") || document.getElementById("flash-canvas") || document.querySelector("canvas");
                    if (!canvas) return;
                    var ctx = canvas.getContext("2d");
                    if (!ctx) return;

                    var colors = "#3498db;#e74c3c;#2ecc71;#f39c12;#9b59b6;#1abc9c".split(";");
                    var color = colors[depth % colors.length];

                    ctx.strokeStyle = color;
                    ctx.lineWidth = 2;

                    var rw = canvas.width - depth * 20;
                    if (rw < 10) rw = 10;
                    var rh = canvas.height - depth * 20;
                    if (rh < 10) rh = 10;

                    ctx.strokeRect(depth * 10, depth * 10, rw, rh);

                    if (strName && strName.length > 0) {
                        ctx.fillStyle = color;
                        ctx.font = "11px monospace";
                        ctx.fillText("obj[" + depth + "] " + strName, depth * 10 + 4, depth * 10 + 14);
                    }
                }), depth, char_id, name.c_str());
#endif
                break;
            }

            case TagID::RemoveObject2: {
                if (tag.length < 2) break;
                uint16_t depth = tag.data[0] | (tag.data[1] << 8);
                g_display_list.removeObject(depth);
                break;
            }

            case TagID::DoAction:
                std::printf("[AVM2] DoAction: %u bytes AS2 bytecode (frame %d)\n",
                            tag.length, g_current_frame);
                break;

            case TagID::DoInitAction:
                std::printf("[AVM2] DoInitAction: %u bytes\n", tag.length);
                break;

            case TagID::DefineSprite:
                std::printf("[AVM2] DefineSprite: %u bytes\n", tag.length);
                break;

            case TagID::ShowFrame:
                std::printf("[AVM2] ShowFrame: frame %d displayed\n",
                            g_current_frame);
                std::printf("[Renderer] Frame rendered (display list: %zu objects)\n",
                            g_display_list.objects().size());
                signalFrameReady();
                break;

            default:
                break;
        }
    }

    g_current_frame++;
    if (g_current_frame >= g_frame_count) {
        g_current_frame = 0; // loop
    }
}

void AVM2Context::shutdown() {
    if (!m_initialized) return;

#ifdef HAS_AVMPLUS
    delete m_core;   m_core = nullptr;
    delete m_gc;     m_gc = nullptr;
    MMgc::GCHeap::Destroy();
#endif

    m_initialized = false;
    std::printf("[AVM2] AVM2 VM shut down\n");
}

} // namespace waflash
