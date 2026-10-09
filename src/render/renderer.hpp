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

#ifndef WAFLASH_RENDER_RENDERER_HPP
#define WAFLASH_RENDER_RENDERER_HPP

namespace waflash {

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool initialize() = 0;
    virtual void beginFrame() = 0;
    virtual void renderStage() = 0;
    virtual void endFrame() = 0;
    virtual void shutdown() = 0;
};

} // namespace waflash

#endif // WAFLASH_RENDER_RENDERER_HPP
