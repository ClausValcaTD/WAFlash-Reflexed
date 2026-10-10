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

#include "swf_loader.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
#include <fstream>

namespace waflash {

SWFLoader::SWFLoader() : m_loaded(false) {
    std::memset(&m_header, 0, sizeof(m_header));
}

SWFLoader::~SWFLoader() {}

bool SWFLoader::load(const std::string& url) {
    std::ifstream file(url, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::printf("[SWFLoader] Failed to open SWF file: %s\n", url.c_str());
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 8) {
        return false;
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }

    return loadFromMemory(buffer.data(), buffer.size());
}

bool SWFLoader::loadFromMemory(const uint8_t* data, size_t size) {
    if (!data || size < 8) {
        m_loaded = false;
        return false;
    }

    m_header.signature[0] = static_cast<char>(data[0]);
    m_header.signature[1] = static_cast<char>(data[1]);
    m_header.signature[2] = static_cast<char>(data[2]);

    bool valid_sig = (m_header.signature[1] == 'W' && m_header.signature[2] == 'S') &&
                     (m_header.signature[0] == 'F' || m_header.signature[0] == 'C' || m_header.signature[0] == 'Z');

    if (!valid_sig) {
        m_loaded = false;
        return false;
    }

    m_header.version = data[3];
    m_header.file_length = static_cast<uint32_t>(data[4]) |
                           (static_cast<uint32_t>(data[5]) << 8) |
                           (static_cast<uint32_t>(data[6]) << 16) |
                           (static_cast<uint32_t>(data[7]) << 24);

    if (m_header.signature[0] == 'F') {
        if (size >= 12) {
            uint8_t nbits = data[8] >> 3;
            size_t rect_bits = 5 + 4 * nbits;
            size_t rect_bytes = (rect_bits + 7) / 8;
            size_t pos = 8 + rect_bytes;

            if (pos + 4 <= size) {
                m_header.frame_rate = static_cast<uint16_t>(data[pos]) | (static_cast<uint16_t>(data[pos+1]) << 8);
                m_header.frame_count = static_cast<uint16_t>(data[pos+2]) | (static_cast<uint16_t>(data[pos+3]) << 8);
            }
        }
    } else {
        // CWS / ZWS header defaults if uncompressed body is not provided
        m_header.frame_rate = 12 << 8; // 12 FPS default
        m_header.frame_count = 1;
    }

    m_loaded = true;
    return true;
}

const SWFHeader& SWFLoader::getHeader() const {
    return m_header;
}

uint8_t SWFLoader::getVersion() const {
    return m_header.version;
}

bool SWFLoader::isAS3() const {
    return m_header.version >= 9;
}

} // namespace waflash
