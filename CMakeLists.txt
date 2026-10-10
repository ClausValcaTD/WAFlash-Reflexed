# Copyright 2025 WAFlash-ReFlexed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16)
project(waflash-reflexed VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    src/
    deps/avmplus/core
    deps/avmplus/MMgc
    deps/avmplus
    deps/swf-parser/include
)

set(ENGINE_SOURCES
    src/core/engine.cpp
    src/core/play_stop.cpp
    src/core/memfs.cpp
    src/swf/swf_loader.cpp
    src/avm/avm2.cpp
    src/render/webgl_renderer.cpp
    src/render/canvas_renderer.cpp
    src/audio/audio.cpp
    src/bridge/external_interface.cpp
    src/memory/allocator.cpp
)

add_executable(waflash-reflexed src/main.cpp ${ENGINE_SOURCES})

# Target compilation options
if(MSVC)
    target_compile_options(waflash-reflexed PRIVATE /W4)
else()
    target_compile_options(waflash-reflexed PRIVATE -Wall -Wextra)
endif()

# Test target
add_executable(test_engine tests/test_engine.cpp ${ENGINE_SOURCES})
