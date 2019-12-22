#include "VGenManager.hpp"

#include "core/AbstractVGen.hpp"

#include "spdlog/spdlog.h"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/yaml.h"

namespace scin {

VGenManager::VGenManager() {}
VGenManager::~VGenManager() {}

int VGenManager::loadFromFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAllFromFile(fileName);
    } catch (const YAML::ParserException&) {
        spdlog::error("error parsing VGen yaml file {}", fileName);
        return -1;
    } catch (const YAML::BadFile&) {
        spdlog::error("bad VGen yaml file {}", fileName);
        return -1;
    }

    return extractFromNodes(nodes);
}

int VGenManager::parseFromString(const std::string& yaml) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAll(yaml);
    } catch (const YAML::ParserException&) {
        spdlog::error("error parsing VGen yaml string {}", yaml);
        return -1;
    }

    return extractFromNodes(nodes);
}

int VGenManager::extractFromNodes(const std::vector<YAML::Node>& nodes) {
    int numberOfValidElements = 0;
    for (auto node : nodes) {
        if (extractFromNode(node))
            ++numberOfValidElements;
    }

    return numberOfValidElements;
}

std::shared_ptr<const AbstractVGen> VGenManager::getVGenNamed(const std::string& name) {
    std::shared_ptr<const AbstractVGen> vgen;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_VGens.find(name);
    if (it != m_VGens.end()) {
        vgen = it->second;
    }
    return vgen;
}

size_t VGenManager::numberOfVGens() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_VGens.size();
}

bool VGenManager::extractFromNode(YAML::Node& node) {
    // Top level structure expected is a Map.
    if (!node.IsMap()) {
        spdlog::error("Top-level yaml node is not a map.");
        return false;
    }
    // Two currently required tags are "name" and "fragment", check they exist.
    if (!node["name"] || !node["fragment"]) {
        spdlog::error("Missing either name or fragment tag.");
        return false;
    }

    std::string name = node["name"].as<std::string>();
    std::string fragment = node["fragment"].as<std::string>();

    std::vector<std::string> inputs;
    if (node["inputs"] && node["inputs"].IsSequence()) {
        for (auto input : node["inputs"]) {
            inputs.push_back(input.as<std::string>());
        }
    }
    std::vector<std::string> parameters;
    if (node["parameters"] && node["parameters"].IsSequence()) {
        for (auto parameter : node["parameters"]) {
            parameters.push_back(parameter.as<std::string>());
        }
    }
    std::vector<std::string> intermediates;
    if (node["intermediates"] && node["intermediates"].IsSequence()) {
        for (auto intermediate : node["intermediates"]) {
            intermediates.push_back(intermediate.as<std::string>());
        }
    }

    std::shared_ptr<AbstractVGen> vgen(new AbstractVGen(name, fragment, inputs, parameters, intermediates));
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_VGens.insert_or_assign(name, vgen);
    }
    return true;
}

} // namespace scin
