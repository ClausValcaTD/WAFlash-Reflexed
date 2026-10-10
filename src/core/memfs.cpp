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

namespace waflash {

bool mountMemFS(const std::string& path) {
#ifdef __EMSCRIPTEN__
    // Create directory in MEMFS
    EM_ASM({
        var path = UTF8ToString($0);
        try {
            FS.mkdir(path);
            console.log('[MEMFS] Created directory: ' + path);
        } catch(e) {
            if (e.code !== 'EEXIST') {
                console.warn('[MEMFS] mkdir failed: ' + e.message);
            }
        }
    }, path.c_str());
#else
    // Native directory creation
    #if defined(_WIN32)
    // windows mkdir
    #else
    mkdir(path.c_str(), 0755);
    #endif
#endif

    std::printf("[MEMFS] Virtual filesystem mounted at '%s'\n", path.c_str());
    return true;
}

} // namespace waflash

void memfs_init(const char* mount_point) {
    if (mount_point) {
        waflash::mountMemFS(mount_point);
    }
}
