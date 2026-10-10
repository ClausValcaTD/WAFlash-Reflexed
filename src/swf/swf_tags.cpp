#include "core/engine.hpp"
// Copyright 2025 WAFlash-ReFlexed Authors
// Apache License 2.0

#include "swf_tags.hpp"
#include <cstdio>
#include <cstring>

namespace waflash {

namespace {

struct BitReader {
    const uint8_t* data;
    size_t bit_pos = 0;

    uint32_t readBits(uint8_t nbits) {
        uint32_t val = 0;
        for (uint8_t i = 0; i < nbits; i++) {
            size_t byte_idx = (bit_pos + i) / 8;
            size_t bit_idx  = 7 - ((bit_pos + i) % 8);
            uint32_t bit = (data[byte_idx] >> bit_idx) & 1;
            val = (val << 1) | bit;
        }
        bit_pos += nbits;
        return val;
    }

    int32_t readSBits(uint8_t nbits) {
        if (nbits == 0) return 0;
        uint32_t val = readBits(nbits);
        if (val & (1U << (nbits - 1))) {
            val |= (~0U) << nbits;
        }
        return static_cast<int32_t>(val);
    }
};

} // namespace

SWFRect SWFTagParser::parseRect(const uint8_t* data, size_t& offset) {
    SWFRect rect = {};
    BitReader reader{ data + offset, 0 };
    uint8_t nbits = static_cast<uint8_t>(reader.readBits(5));
    rect.xmin = reader.readSBits(nbits);
    rect.xmax = reader.readSBits(nbits);
    rect.ymin = reader.readSBits(nbits);
    rect.ymax = reader.readSBits(nbits);
    offset += (reader.bit_pos + 7) / 8;
    return rect;
}

bool SWFTagParser::parseHeader(const uint8_t* body, size_t size) {
    if (!body || size < 4) return false;

    size_t offset = 0;

    // Parse RECT (frame size)
    m_frame_size = parseRect(body, offset);

    // Frame rate (fixed point 8.8)
    if (offset + 4 > size) return false;
    uint16_t raw_rate = body[offset] | (body[offset+1] << 8);
    m_frame_rate = raw_rate / 256.0f;
    offset += 2;

    // Frame count
    m_frame_count = body[offset] | (body[offset+1] << 8);
    offset += 2;

    m_tags_offset = offset;
    setSWFDimensions(static_cast<int>(m_frame_size.width()), static_cast<int>(m_frame_size.height()));

    std::printf("[SWFTagParser] Frame: %.0fx%.0f @ %.1f fps, %d frames\n",
                m_frame_size.width(), m_frame_size.height(),
                m_frame_rate, m_frame_count);

    return true;
}

bool SWFTagParser::parse(const uint8_t* body, size_t size, TagCallback callback) {
    if (!parseHeader(body, size)) return false;

    size_t offset = m_tags_offset;

    while (offset + 2 <= size) {
        // Read record header (2 bytes)
        uint16_t record = body[offset] | (body[offset+1] << 8);
        offset += 2;

        uint16_t raw_id    = record >> 6;
        uint32_t tag_len   = record & 0x3F;

        // Long tag format
        if (tag_len == 0x3F) {
            if (offset + 4 > size) break;
            tag_len = body[offset]       |
                     (body[offset+1]<<8) |
                     (body[offset+2]<<16)|
                     (body[offset+3]<<24);
            offset += 4;
        }

        if (offset + tag_len > size) break;

        SWFTag tag;
        tag.raw_id = raw_id;
        tag.id     = static_cast<TagID>(raw_id);
        tag.length = tag_len;
        tag.data   = body + offset;

        // Handle SetBackgroundColor inline
        if (tag.id == TagID::SetBackgroundColor && tag_len >= 3) {
            m_bg_color = { tag.data[0], tag.data[1], tag.data[2], 255 };
            setBackgroundColor(tag.data[0], tag.data[1], tag.data[2]);
            std::printf("[SWFTagParser] Background: rgb(%d,%d,%d)\n",
                        m_bg_color.r, m_bg_color.g, m_bg_color.b);
        }

        if (callback) {
            callback(tag);
        }

        if (tag.id == TagID::End) break;
        offset += tag_len;
    }

    return true;
}

} // namespace waflash
