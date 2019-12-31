#include "core/Shape.hpp"

namespace scin {

Quad::~Quad() {
}

Manifest::ElementType Quad::elementType() {
    return Manifest::ElementType::kVec2;
}

} // namespace scin
