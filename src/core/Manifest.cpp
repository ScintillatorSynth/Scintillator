#include "core/Manifest.hpp"

#include "spdlog/spdlog.h"

namespace scin {

Manifest::Manifest(): m_numberOfElements(0), m_size(0) {}

Manifest::~Manifest() {}

bool Manifest::addElement(const std::string& name, ElementType type) {
    return true;
}

} // namespace scin
