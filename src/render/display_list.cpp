// Copyright 2025 WAFlash-ReFlexed Authors
// Apache License 2.0

#include "display_list.hpp"

namespace waflash {

void DisplayList::placeObject(uint16_t depth, uint16_t char_id,
                               const std::string& name, const Matrix2D& matrix) {
    DisplayObject obj;
    obj.depth        = depth;
    obj.character_id = char_id;
    obj.name         = name;
    obj.matrix       = matrix;
    obj.visible      = true;
    m_objects[depth] = obj;
}

void DisplayList::removeObject(uint16_t depth) {
    m_objects.erase(depth);
}

void DisplayList::clear() {
    m_objects.clear();
}

} // namespace waflash
