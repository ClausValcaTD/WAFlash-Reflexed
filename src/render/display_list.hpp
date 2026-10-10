// Copyright 2025 WAFlash-ReFlexed Authors
// Apache License 2.0

#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace waflash {

struct Matrix2D {
    float a = 1, b = 0;
    float c = 0, d = 1;
    float tx = 0, ty = 0;
};

struct DisplayObject {
    uint16_t    depth       = 0;
    uint16_t    character_id = 0;
    std::string name;
    Matrix2D    matrix;
    bool        visible     = true;
};

// Represents one frame's display list
// Built from PlaceObject2/RemoveObject2 tags
class DisplayList {
public:
    void placeObject(uint16_t depth, uint16_t char_id,
                     const std::string& name = "",
                     const Matrix2D& matrix = {});

    void removeObject(uint16_t depth);
    void clear();

    const std::map<uint16_t, DisplayObject>& objects() const {
        return m_objects;
    }

private:
    std::map<uint16_t, DisplayObject> m_objects; // keyed by depth
};

} // namespace waflash
