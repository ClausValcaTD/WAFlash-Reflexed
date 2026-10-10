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

#ifndef WAFLASH_SWF_SWF_LOADER_HPP
#define WAFLASH_SWF_SWF_LOADER_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace waflash {

struct SWFHeader {
    char     signature[3];  // "FWS", "CWS", or "ZWS"
    uint8_t  version;
    uint32_t file_length;
    uint16_t frame_rate;
    uint16_t frame_count;
};

class SWFLoader {
public:
    SWFLoader();
    ~SWFLoader();

    bool load(const std::string& url);
    bool loadFromMemory(const uint8_t* data, size_t size);
    const SWFHeader& getHeader() const;
    uint8_t getVersion() const;
    bool isAS3() const; // SWF version >= 9
    bool isCompressed() const;
    const std::vector<uint8_t>& getDecompressedBody() const;

private:
    SWFHeader m_header;
    bool m_loaded;
    std::vector<uint8_t> m_decompressed_body; // stores FWS/CWS/ZWS body
};

} // namespace waflash

#endif // WAFLASH_SWF_SWF_LOADER_HPP
