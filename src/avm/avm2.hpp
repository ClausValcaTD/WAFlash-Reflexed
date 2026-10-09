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

#ifndef WAFLASH_AVM_AVM2_HPP
#define WAFLASH_AVM_AVM2_HPP

#include <string>

namespace waflash {

class AVM2Context {
public:
    AVM2Context();
    ~AVM2Context();

    bool init();
    bool loadSWF(const std::string& url);
    void executeFrame();
    void shutdown();

private:
    bool m_initialized;
};

} // namespace waflash

#endif // WAFLASH_AVM_AVM2_HPP
