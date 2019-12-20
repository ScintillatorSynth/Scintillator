#include "ScinthDefManager.hpp"

#include "VGenManager.hpp"

namespace scin {

ScinthDefManager::ScintDefManager(std::shared_ptr<VGenManager> vgenManager) : m_vgenManager(vgenManager) { }

ScinthDefManager::~ScinthDefManager() { }

int ScinthDefManager::loadFromFile(const std::string& fileName) {
    return 0;
}

int ScinthDefManager::parseFromString(const std::string& yaml) {
    return 0;
}

int ScinthDefManager::extractFromNodes(const std::vector<YAML::Node>& nodes) {
    return 0;
}

}  // namespace scin

