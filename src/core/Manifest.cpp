#include "core/Manifest.hpp"

#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

namespace scin {

Manifest::Manifest(): m_size(0) {}

Manifest::~Manifest() {}

bool Manifest::addElement(const std::string& name, ElementType type, Intrinsic intrinsic) {
    if (m_types.find(name) != m_types.end()) {
        spdlog::error("duplicate addition to Manifest of {}", name);
        return false;
    }

    m_types.insert({ name, type });
    m_intrinsics.insert({ name, intrinsic });
    return true;
}

void Manifest::pack() {
    // Bucket the elements by size, so we can start packing.
    std::vector<std::string> floatElements;
    std::vector<std::string> vec2Elements;
    std::vector<std::string> vec3Elements;
    std::vector<std::string> vec4Elements;

    for (auto it : m_types) {
        switch (it.second) {
        case kFloat:
            floatElements.push_back(it.first);
            break;

        case kVec2:
            vec2Elements.push_back(it.first);
            break;

        case kVec3:
            vec3Elements.push_back(it.first);
            break;

        case kVec4:
            vec4Elements.push_back(it.first);
            break;
        }
    }

    // Pack elements biggest to smallest.
    for (auto element : vec4Elements) {
        packElement(element, sizeof(glm::vec4));
    }

    if (vec3Elements.size()) {
        // Check padding for alignment with vec3.
        uint32_t padding = sizeof(glm::vec3) - (m_size % sizeof(glm::vec3));
        if (padding < sizeof(glm::vec3)) {
            // We can pack a vec2 in the padding if there's room and it's aligned.
            if (padding >= sizeof(glm::vec2) && ((m_size % sizeof(glm::vec2)) == 0) && vec2Elements.size()) {
                packElement(*vec2Elements.rbegin(), sizeof(glm::vec2));
                vec2Elements.pop_back();
                padding -= sizeof(glm::vec2);
            }
            // See if there's room and available floats to pack in as well (could be as many as two).
            packFloats(padding, floatElements);
            m_size += padding;
        }
        for (auto element : vec3Elements) {
            packElement(element, sizeof(glm::vec3));
        }
    }

    if (vec2Elements.size()) {
        uint32_t padding = sizeof(glm::vec2) - (m_size % sizeof(glm::vec2));
        if (padding < sizeof(glm::vec2)) {
            packFloats(padding, floatElements);
            m_size += padding;
        }
        for (auto element : vec2Elements) {
            packElement(element, sizeof(glm::vec2));
        }
    }

    for (auto element : floatElements) {
        packElement(element, sizeof(float));
    }
}

const std::string Manifest::typeNameForElement(size_t index) const {
    ElementType type = typeForElement(index);
    switch (type) {
    case kFloat:
        return std::string("float");

    case kVec2:
        return std::string("vec2");

    case kVec3:
        return std::string("vec3");

    case kVec4:
        return std::string("vec4");
    }

    return "unknown type";
}

uint32_t Manifest::strideForElement(size_t index) const {
    if (index == m_names.size() - 1) {
        return m_size - offsetForElement(index);
    }
    return offsetForElement(index + 1) - offsetForElement(index);
}

void Manifest::packFloats(uint32_t& padding, std::vector<std::string>& floatElements) {
    while (padding >= sizeof(float) && ((m_size % sizeof(float)) == 0) && floatElements.size()) {
        packElement(*floatElements.rbegin(), sizeof(float));
        floatElements.pop_back();
        padding -= sizeof(float);
    }
}

void Manifest::packElement(const std::string& name, uint32_t size) {
    m_names.push_back(name);
    m_offsets.insert({ name, m_size });
    m_size += size;
}

} // namespace scin
