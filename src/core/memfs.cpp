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

#include "memfs.hpp"
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

void memfs_init(const char* mount_point) {
    // Mount MEMFS directory for SharedObjects (/waflashso)
#ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            FS.mkdir(UTF8ToString($0));
            FS.mount(MEMFS, {}, UTF8ToString($0));
        } catch (e) {
            // Directory might already exist
        }
    }, mount_point);
#else
    // Native directory creation
    #if defined(_WIN32)
    // windows mkdir
    #else
    mkdir(mount_point, 0755);
    #endif
#endif
    std::printf("[MEMFS] Virtual filesystem mounted at '%s'\n", mount_point);
}
