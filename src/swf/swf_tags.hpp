// Copyright 2025 WAFlash-ReFlexed Authors
// Apache License 2.0

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace waflash {

// SWF Tag IDs (AS2 subset needed for Hoshi Saga)
enum class TagID : uint16_t {
    End                 = 0,
    ShowFrame           = 1,
    DefineShape         = 2,
    PlaceObject         = 4,
    RemoveObject        = 5,
    DefineBits          = 6,
    DefineButton        = 7,
    SetBackgroundColor  = 9,
    DoAction            = 12,
    DefineFont          = 13,
    DefineText          = 11,
    DefineSound         = 14,
    SoundStreamHead     = 18,
    SoundStreamBlock    = 19,
    DefineBitsLossless  = 20,
    DefineBitsJPEG2     = 21,
    DefineShape2        = 22,
    PlaceObject2        = 26,
    RemoveObject2       = 28,
    DefineShape3        = 32,
    DefineText2         = 33,
    DefineButton2       = 34,
    DefineBitsJPEG3     = 35,
    DefineBitsLossless2 = 36,
    DefineSprite        = 39,
    NameCharacter       = 40,
    DoInitAction        = 59,
    PlaceObject3        = 70,
    Unknown             = 0xFFFF
};

struct SWFTag {
    TagID           id;
    uint16_t        raw_id;
    uint32_t        length;
    const uint8_t*  data;   // pointer into decompressed SWF body
};

struct RGBA {
    uint8_t r, g, b, a;
};

struct SWFRect {
    int32_t xmin, xmax, ymin, ymax; // in twips (1/20 pixel)
    float width()  const { return (xmax - xmin) / 20.0f; }
    float height() const { return (ymax - ymin) / 20.0f; }
};

// Callback fired for each tag during parsing
using TagCallback = std::function<void(const SWFTag&)>;

class SWFTagParser {
public:
    SWFTagParser() = default;

    // Parse all tags from decompressed SWF body
    // body: pointer to bytes AFTER the 8-byte SWF file header
    // size: length of body in bytes
    bool parse(const uint8_t* body, size_t size, TagCallback callback);

    // Parse just header info (background color, frame size, fps)
    bool parseHeader(const uint8_t* body, size_t size);

    // Parsed header values
    RGBA     backgroundColor() const { return m_bg_color; }
    SWFRect  frameSize()       const { return m_frame_size; }
    float    frameRate()       const { return m_frame_rate; }
    uint16_t frameCount()      const { return m_frame_count; }
    size_t   tagsOffset()      const { return m_tags_offset; }

private:
    RGBA     m_bg_color    = {255, 255, 255, 255};
    SWFRect  m_frame_size  = {};
    float    m_frame_rate  = 24.0f;
    uint16_t m_frame_count = 1;
    size_t   m_tags_offset = 0;

    static SWFRect parseRect(const uint8_t* data, size_t& offset);
};

} // namespace waflash
