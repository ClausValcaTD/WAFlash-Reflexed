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
#include <zlib.h>

#ifdef HAS_LZMA
#include <lzma.h>
#endif

namespace waflash {

// CWS = zlib compressed (Flash 6+)
static bool decompress_zlib(const uint8_t* src, size_t src_size,
                            std::vector<uint8_t>& out, uint32_t uncompressed_size) {
    out.resize(uncompressed_size);
    uLongf dest_len = uncompressed_size;

    int result = uncompress(
        out.data(), &dest_len,
        src, static_cast<uLong>(src_size)
    );

    if (result == Z_OK) {
        out.resize(dest_len);
        return true;
    }
    return false;
}

// ZWS = LZMA compressed (Flash 13+)
#ifdef HAS_LZMA
static bool decompress_lzma(const uint8_t* src, size_t src_size,
                            std::vector<uint8_t>& out, uint32_t uncompressed_size) {
    out.resize(uncompressed_size);

    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX);
    if (ret != LZMA_OK) return false;

    strm.next_in  = src;
    strm.avail_in = src_size;
    strm.next_out = out.data();
    strm.avail_out = uncompressed_size;

    ret = lzma_code(&strm, LZMA_FINISH);
    lzma_end(&strm);

    return (ret == LZMA_OK || ret == LZMA_STREAM_END);
}
#endif

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

    char sig0 = static_cast<char>(data[0]);
    char sig1 = static_cast<char>(data[1]);
    char sig2 = static_cast<char>(data[2]);

    bool valid = (sig1 == 'W' && sig2 == 'S') &&
                 (sig0 == 'F' || sig0 == 'C' || sig0 == 'Z');
    if (!valid) {
        m_loaded = false;
        return false;
    }

    m_header.signature[0] = sig0;
    m_header.signature[1] = sig1;
    m_header.signature[2] = sig2;
    m_header.version = data[3];
    m_header.file_length = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);

    // Decompress body if needed
    m_decompressed_body.clear();
    const uint8_t* body = data + 8;
    size_t body_size = size - 8;

    if (sig0 == 'C') {
        // CWS: zlib compressed body
        uint32_t uncompressed = m_header.file_length - 8;
        if (!decompress_zlib(body, body_size, m_decompressed_body, uncompressed)) {
            std::printf("[SWFLoader] zlib decompression failed\n");
            m_loaded = false;
            return false;
        }
        body = m_decompressed_body.data();
        body_size = m_decompressed_body.size();
        std::printf("[SWFLoader] CWS decompressed: %zu bytes\n", m_decompressed_body.size());

    } else if (sig0 == 'Z') {
#ifdef HAS_LZMA
        // ZWS: LZMA compressed — skip 4-byte uncompressed size header
        uint32_t uncompressed = m_header.file_length - 8;
        if (body_size < 4) {
            m_loaded = false;
            return false;
        }
        if (!decompress_lzma(body + 4, body_size - 4, m_decompressed_body, uncompressed)) {
            std::printf("[SWFLoader] LZMA decompression failed\n");
            m_loaded = false;
            return false;
        }
        body = m_decompressed_body.data();
        body_size = m_decompressed_body.size();
        std::printf("[SWFLoader] ZWS decompressed: %zu bytes\n", m_decompressed_body.size());
#else
        std::printf("[SWFLoader] ZWS (LZMA) not supported in this build\n");
        m_loaded = false;
        return false;
#endif
    } else {
        // FWS: uncompressed, store body into m_decompressed_body for consistency
        m_decompressed_body.assign(body, body + body_size);
        body = m_decompressed_body.data();
    }

    // Parse RECT (FrameSize) from decompressed body
    if (body_size >= 4) {
        uint8_t nbits = body[0] >> 3;
        size_t rect_bits = 5 + 4 * nbits;
        size_t rect_bytes = (rect_bits + 7) / 8;
        size_t pos = rect_bytes;

        if (pos + 4 <= body_size) {
            m_header.frame_rate  = body[pos]   | (body[pos+1] << 8);
            m_header.frame_count = body[pos+2] | (body[pos+3] << 8);
        }
    }

    m_loaded = true;
    std::printf("[SWFLoader] SWF v%d loaded: %u frames @ %.1f fps\n",
        m_header.version,
        m_header.frame_count,
        m_header.frame_rate / 256.0f);

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

bool SWFLoader::isCompressed() const {
    return m_header.signature[0] == 'C' || m_header.signature[0] == 'Z';
}

const std::vector<uint8_t>& SWFLoader::getDecompressedBody() const {
    return m_decompressed_body;
}

} // namespace waflash
