#ifndef SRC_VGEN_MANAGER_HPP_
#define SRC_VGEN_MANAGER_HPP_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace YAML {
class Node;
}

namespace scin {

class VGen;

// Manages VGens, can parse them from YAML, keeps the master list of them.
class VGenManager {
public:
    VGenManager();
    ~VGenManager();

    // Convenience routine when the VGen yaml is in a file (mostly for the built-in VGens). Returns the number of
    // VGens parsed, or a negative number on error.
    int loadFromFile(const std::string& fileName);
    // Parse a VGen from a string.
    int parseFromString(const std::string& yaml);

    // mutex hit
    std::shared_ptr<VGen> getVGenNamed(const std::string& name);
    // mutex hit
    size_t numberOfVGens();

private:
    int extractFromNodes(const std::vector<YAML::Node>& nodes);
    // Extracts a single VGen from a parsed YAML node.
    bool extractFromNode(YAML::Node& node);

    std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<VGen>> m_VGens;
};

}  // namespace scin

#endif  // SRC_VGEN_MANAGER_HPP_

