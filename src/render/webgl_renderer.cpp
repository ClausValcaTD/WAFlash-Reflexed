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

#include "webgl_renderer.hpp"
#include <cstdio>

namespace waflash {

WebGLRenderer::WebGLRenderer(bool enableFilters)
    : m_enableFilters(enableFilters), m_initialized(false) {}

WebGLRenderer::~WebGLRenderer() {
    shutdown();
}

bool WebGLRenderer::initialize() {
    std::printf("[WebGLRenderer] Initializing WebGL backend (filters: %s)\n",
                m_enableFilters ? "ON" : "OFF");
    m_initialized = true;
    return true;
}

void WebGLRenderer::beginFrame() {
    if (!m_initialized) return;
}

void WebGLRenderer::renderStage() {
    if (!m_initialized) return;
}

void WebGLRenderer::endFrame() {
    if (!m_initialized) return;
}

void WebGLRenderer::shutdown() {
    if (m_initialized) {
        m_initialized = false;
        std::printf("[WebGLRenderer] WebGL backend shut down\n");
    }
}

} // namespace waflash
