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
    deps/swf-parser/include
)

find_package(ZLIB REQUIRED)

find_package(LibLZMA)

set(ENGINE_SOURCES
    src/core/engine.cpp
    src/core/play_stop.cpp
    src/core/memfs.cpp
    src/swf/swf_loader.cpp
    src/avm/avm2.cpp
    src/avm/movie_clip.cpp
    src/render/webgl_renderer.cpp
    src/render/canvas_renderer.cpp
    src/audio/audio.cpp
    src/bridge/external_interface.cpp
    src/memory/allocator.cpp
)

set(AVMPLUS_DIR ${CMAKE_SOURCE_DIR}/deps/avmplus)
set(ENABLE_AVMPLUS OFF)

# Note: avmplus requires a complex build toolchain (configure.py/avmfeatures) to generate headers like avmhost-features.h.
# When buildable, target_compile_definitions adds HAS_AVMPLUS=1. Otherwise fallback to clean #ifdef stub.
if(EXISTS ${AVMPLUS_DIR}/core/AvmCore.cpp AND EXISTS ${AVMPLUS_DIR}/generated/avmhost-features.h)
    set(AVMPLUS_SOURCES
        ${AVMPLUS_DIR}/core/AvmCore.cpp
        ${AVMPLUS_DIR}/core/Toplevel.cpp
        ${AVMPLUS_DIR}/core/ScriptObject.cpp
        ${AVMPLUS_DIR}/core/ClassClosure.cpp
        ${AVMPLUS_DIR}/core/FunctionClass.cpp
        ${AVMPLUS_DIR}/core/ArrayClass.cpp
        ${AVMPLUS_DIR}/core/StringClass.cpp
        ${AVMPLUS_DIR}/MMgc/GC.cpp
        ${AVMPLUS_DIR}/MMgc/GCHeap.cpp
    )

    add_library(avmplus STATIC ${AVMPLUS_SOURCES})

    target_include_directories(avmplus PUBLIC
        ${AVMPLUS_DIR}
        ${AVMPLUS_DIR}/core
        ${AVMPLUS_DIR}/MMgc
        ${AVMPLUS_DIR}/VMPI
        ${AVMPLUS_DIR}/AVMPI
        ${AVMPLUS_DIR}/pcre
        ${AVMPLUS_DIR}/generated
    )

    target_compile_definitions(avmplus PUBLIC
        VMCFG_EVAL=1
        AVMPLUS_LITTLE_ENDIAN=1
    )

    set(ENABLE_AVMPLUS ON)
endif()

add_executable(waflash-reflexed src/main.cpp ${ENGINE_SOURCES})
target_link_libraries(waflash-reflexed PRIVATE ZLIB::ZLIB)

if(LIBLZMA_FOUND)
    target_link_libraries(waflash-reflexed PRIVATE LibLZMA::LibLZMA)
    target_compile_definitions(waflash-reflexed PRIVATE HAS_LZMA=1)
endif()

if(ENABLE_AVMPLUS)
    target_link_libraries(waflash-reflexed PRIVATE avmplus)
    target_compile_definitions(waflash-reflexed PRIVATE HAS_AVMPLUS=1)
endif()

# Target compilation options
if(MSVC)
    target_compile_options(waflash-reflexed PRIVATE /W4)
else()
    target_compile_options(waflash-reflexed PRIVATE -Wall -Wextra)
endif()

# Test target
add_executable(test_engine tests/test_engine.cpp ${ENGINE_SOURCES})
target_link_libraries(test_engine PRIVATE ZLIB::ZLIB)

if(LIBLZMA_FOUND)
    target_link_libraries(test_engine PRIVATE LibLZMA::LibLZMA)
    target_compile_definitions(test_engine PRIVATE HAS_LZMA=1)
endif()

if(ENABLE_AVMPLUS)
    target_link_libraries(test_engine PRIVATE avmplus)
    target_compile_definitions(test_engine PRIVATE HAS_AVMPLUS=1)
endif()

# Hoshi Saga Test Target
add_executable(test_hoshi
    tests/test_hoshi.cpp
    src/avm/movie_clip.cpp
    src/swf/swf_loader.cpp
    src/core/engine.cpp
    src/core/play_stop.cpp
    src/core/memfs.cpp
    src/avm/avm2.cpp
    src/audio/audio.cpp
    src/memory/allocator.cpp
)
target_include_directories(test_hoshi PRIVATE src/)
target_link_libraries(test_hoshi PRIVATE ZLIB::ZLIB)

if(LIBLZMA_FOUND)
    target_link_libraries(test_hoshi PRIVATE LibLZMA::LibLZMA)
    target_compile_definitions(test_hoshi PRIVATE HAS_LZMA=1)
endif()

if(ENABLE_AVMPLUS)
    target_link_libraries(test_hoshi PRIVATE avmplus)
    target_compile_definitions(test_hoshi PRIVATE HAS_AVMPLUS=1)
endif()
