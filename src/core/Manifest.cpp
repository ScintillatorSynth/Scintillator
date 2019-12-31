#include "core/Manifest.hpp"

#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

namespace scin {

Manifest::Manifest(): m_size(0) {}

Manifest::~Manifest() {}

bool Manifest::addElement(const std::string& name, ElementType type) {
    if (m_names.find(name) != m_names.end()) {
        spdlog::error("duplicate addition to Manifest of {}", name);
        return false;
    }

    return true;
}

void Manifest::pack() {
    // Bucket the elements by size, so we can start packing.
    std::vector<std::string> floatElements;
    std::vector<std::string> vec2Elements;
    std::vector<std::string> vec3Elements;
    std::vector<std::string> vec4Elements;

    for (auto it : m_names) {
        switch (it->second) {
            case kFloat:
                floatElements.push_back(it->first);
                break;

            case kVec2:
                vec2Elements.push_back(it->first);
                break;

            case kVec3:
                vec3Elements.push_back(it->first);
                break;

            case kVec4:
                vec4Elements.push_back(it->first);
                break;
        }
    }

    // Pack elements biggest to smallest.
    for (auto element : vec4Elements) {
        m_names.push_back(element);
        m_offsets.insert({ element, m_size });
        m_size += sizeof(glm::vec4);
    }

    if (vec3Elements.size()) {
        // Check padding for alignment with vec3.
        uint32_t padding = sizeof(glm::vec3) - (m_size % sizeof(glm::vec3));
        if (padding < sizeof(glm::vec3)) {
            // We can pack a vec2 in the padding if there's room and it's aligned.
            if (padding >= sizeof(glm::vec2) && ((m_size % sizeof(glm::vec2)) == 0) && vec2Elements.size()) {
                m_names.push_back(element);
                m_offsets.insert({ *vec2Elements.rbegin(), m_size });
                vec2Elements.pop_back();
                m_size += sizeof(glm::vec2);
                padding -= sizeof(glm::vec2);
            }
            // See if there's room and available floats to pack in as well (could be as many as two).
            packFloats(padding, floatElements);
            m_size += padding;
        }
        for (auto element : vec3Elements) {
            m_names.push_back(element);
            m_offsets.insert({ element, m_size });
            m_size += sizeof(glm::vec3);
        }
    }

    if (vec2Elements.size()) {
        uint32_t padding = sizeof(glm::vec2) - (m_size % sizeof(glm::vec2));
        if (padding < sizeof(glm::vec2)) {
            packFloats(padding, floatElements);
            m_size += padding;
        }
        for (auto element : vec2Elements) {
            m_offset.insert({ element, m_size });
            m_size += sizeof(glm::vec2);
        }
    }

    for (auto element : floatElements) {
        m_names.push_back(element);
        m_offsets.insert({ element, m_size });
        m_size += sizeof(float);
    }
}

void Manifest::packFloats(uint32_t& padding, std::vector<std::string>& floatElements) {
    while (padding >= sizeof(float) && ((m_size % sizeof(float)) == 0) && floatElements.size()) {
        m_names.push_back(element);
        m_offsets.insert({ *floatElements.rbegin(), m_size });
        floatElements.pop_back();
        m_size += sizeof(float);
        padding -= sizeof(float);
    }
}

} // namespace scin
