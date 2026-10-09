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

#ifndef WAFLASH_MEMORY_ALLOCATOR_HPP
#define WAFLASH_MEMORY_ALLOCATOR_HPP

#include <cstddef>
#include <cstdint>

// dlmalloc chunk header prefix spec:
// Low bits: bit 0 = PREV_INUSE, bit 1 = IS_MMAPPED
struct ChunkHeader {
    size_t prev_size;
    size_t size;
};

void* waflash_malloc(size_t size);
void waflash_free(void* ptr);
void* waflash_memalign(size_t alignment, size_t size);

#endif // WAFLASH_MEMORY_ALLOCATOR_HPP
