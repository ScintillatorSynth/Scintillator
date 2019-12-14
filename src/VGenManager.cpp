#include "VGenManager.hpp"

#include "spdlog/spdlog.h"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/yaml.h"

namespace scin {

VGenManager::VGenManager() {
}
VGenManager::~VGenManager() {
}

bool VGenManager::loadFromFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAllFromFile(fileName);
    } catch (const YAML::ParserException&) {
        spdlog::error("error parsing VGen yaml file {}", fileName);
        return false;
    } catch (const YAML::BadFile&) {
        spdlog::error("bad file {}", fileName);
    }

    for (auto node : nodes) {
        if (!extractFromNode(node)) {
            return false;
        }
    }

    return true;
}

bool VGenManager::parseFromString(const std::string& yaml) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAll(yaml);
    } catch (const YAML::ParserException&) {
        spdlog::error("error parsing VGen yaml string {}", yaml);
        return false;
    }

    for (auto node : nodes) {
        if (!extractFromNode(node)) {
            return false;
        }
    }

    return true;
}

bool VGenManager::extractFromNode(YAML::Node& node) {
    // Top level structure expected is a Map.
    if (!node.IsMap()) {
        spdlog::error("Top-level yaml node is not a map.");
        return false;
    }

    // Extract each element from the sequence.
    return true;
}

}  // namespace scin

