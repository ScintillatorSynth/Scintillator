#ifndef SRC_VGEN_MANAGER_HPP_
#define SRC_VGEN_MANAGER_HPP_

#include "VGen.hpp"

#include <unordered_map>

namespace YAML {
class Node;
}

namespace scin {

// Manages VGens, can parse them from YAML, keeps the master list of them.
class VGenManager {
public:
    VGenManager();
    ~VGenManager();

    // Convenience routine when the VGen yaml is in a file (mostly for the built-in VGens)
    bool loadFromFile(const std::string& fileName);
    // Parse a VGen from a string.
    bool parseFromString(const std::string& yaml);

private:
    bool extractFromNode(YAML::Node& node);

    std::unordered_map<std::string, VGen> m_VGens;
};

}  // namespace scin

#endif  // SRC_VGEN_MANAGER_HPP_

