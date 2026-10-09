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

#include "allocator.hpp"
#include <cstdlib>
#include <cstring>

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
#include <stdlib.h>
#endif

void* waflash_malloc(size_t size) {
    // dlmalloc specification:
    // Smallbins: size <= 244 bytes (O(1) fast-path)
    // Treebins: size > 244 bytes
    return std::malloc(size);
}

void waflash_free(void* ptr) {
    if (ptr) {
        std::free(ptr);
    }
}

void* waflash_memalign(size_t alignment, size_t size) {
    // 16-byte alignment required for SIMD and WebGL buffers
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }

#if defined(__EMSCRIPTEN__)
    // Emscripten builtin memalign
    extern "C" void* emscripten_builtin_memalign(size_t alignment, size_t size);
    return emscripten_builtin_memalign(alignment, size);
#elif defined(_MSC_VER)
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#endif
}
