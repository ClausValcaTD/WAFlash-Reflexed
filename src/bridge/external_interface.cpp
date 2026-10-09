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

#include "external_interface.hpp"
#include <cstdio>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

extern "C" {

// AS -> JS ExternalInterface callback dispatcher
// Lookup key: "_callbacks"
// Dispatch method: "call"
int invokeExternalCallback(void* funcName, void* args) {
    std::printf("[ExternalInterface] Invoking external callback\n");

#ifdef __EMSCRIPTEN__
    // In Emscripten, lookup in window._callbacks and invoke call
    return EM_ASM_INT({
        var cbKey = "_callbacks";
        var func = UTF8ToString($0);
        if (window[cbKey] && typeof window[cbKey][func] === 'function') {
            return window[cbKey][func].call(null, $1) || 0;
        }
        return 0;
    }, funcName, args);
#else
    (void)funcName;
    (void)args;
    return 0;
#endif
}

}
