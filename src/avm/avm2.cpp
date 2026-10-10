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
#include <cstdio>
#include <cstdlib>

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

#ifdef HAS_AVMPLUS
    // Load SWF bytecode into avmplus
    std::printf("[AVM2] Loading SWF into real AvmCore: %s\n", url.c_str());
    // TODO: parse ABC bytecode from SWF tags and feed to AvmCore
#else
    std::printf("[AVM2] Stub: Loading SWF '%s'\n", url.c_str());
#endif

    return true;
}

void AVM2Context::executeFrame() {
    if (!m_initialized) return;

#ifdef HAS_AVMPLUS
    if (m_core) {
        // Execute pending ActionScript operations
        // m_core->executeTimeout() or equivalent frame tick
    }
#endif
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
