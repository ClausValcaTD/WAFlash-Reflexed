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

#include "core/engine.hpp"
#include "core/memfs.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

uint8_t g_disable_filters = 0;

int main(int argc, char** argv) {
    // Stack-allocated buffer capped at 4,095 bytes for SWF URL
    char swf_url[4096];
    std::memset(swf_url, 0, sizeof(swf_url));

    if (argc > 1 && argv[1]) {
        size_t len = std::strlen(argv[1]);
        size_t copy_len = (std::min)(len, static_cast<size_t>(4095));
        std::strncpy(swf_url, argv[1], copy_len);
        swf_url[copy_len] = '\0';
    } else {
        std::strncpy(swf_url, "default.swf", 4095);
    }

    bool use_webgl = false;
    if (argc >= 4 && argv[3]) {
        if (std::strcmp(argv[3], "webgl") == 0) {
            use_webgl = true;
        }
    }

    if (argc >= 5 && argv[4]) {
        if (argv[4][0] == '1') { // ASCII 49
            g_disable_filters = 1;
        }
    }

    const char* mode_str = use_webgl ? "WebGL" : "Canvas2D";
    std::printf("WAFlash-ReFlexed v0.1.0 (%s)\n", mode_str);

    memfs_init("/waflashso");
    engine_init(swf_url, use_webgl, g_disable_filters != 0);

    return 0;
}
