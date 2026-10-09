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

#include "canvas_renderer.hpp"
#include <cstdio>

namespace waflash {

CanvasRenderer::CanvasRenderer() : m_initialized(false) {}

CanvasRenderer::~CanvasRenderer() {
    shutdown();
}

bool CanvasRenderer::initialize() {
    std::printf("[CanvasRenderer] Initializing 2D Canvas fallback backend\n");
    m_initialized = true;
    return true;
}

void CanvasRenderer::beginFrame() {
    if (!m_initialized) return;
}

void CanvasRenderer::renderStage() {
    if (!m_initialized) return;
}

void CanvasRenderer::endFrame() {
    if (!m_initialized) return;
}

void CanvasRenderer::shutdown() {
    if (m_initialized) {
        m_initialized = false;
        std::printf("[CanvasRenderer] 2D Canvas backend shut down\n");
    }
}

} // namespace waflash
