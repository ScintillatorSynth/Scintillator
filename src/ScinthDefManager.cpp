#include "ScinthDefManager.hpp"

#include "ScinthDef.hpp"
#include "VGenManager.hpp"
#include "core/AbstractVGen.hpp"
#include "core/VGen.hpp"

#include "spdlog/spdlog.h"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/yaml.h"

namespace scin {

ScinthDefManager::ScinthDefManager(std::shared_ptr<VGenManager> vgenManager): m_vgenManager(vgenManager) {}

ScinthDefManager::~ScinthDefManager() {}

int ScinthDefManager::loadFromFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAllFromFile(fileName);
    } catch (const YAML::ParserException&) {
        spdlog::error("error parsing ScinthDef yaml file {}", fileName);
        return -1;
    } catch (const YAML::BadFile&) {
        spdlog::error("bad ScinthDef yaml file {}", fileName);
        return -1;
    }

    return extractFromNodes(nodes);
}

int ScinthDefManager::parseFromString(const std::string& yaml) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAll(yaml);
    } catch (const YAML::ParserException&) {
        spdlog::error("error parsing ScinthDef yaml string {}", yaml);
        return -1;
    }

    return extractFromNodes(nodes);
}

std::shared_ptr<const ScinthDef> ScinthDefManager::getScinthDefNamed(const std::string& name) {
    std::shared_ptr<const ScinthDef> scinthDef;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_scinthDefs.find(name);
    if (it != m_scinthDefs.end()) {
        scinthDef = it->second;
    }
    return scinthDef;
}

size_t ScinthDefManager::numberOfScinthDefs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_scinthDefs.size();
}

int ScinthDefManager::extractFromNodes(const std::vector<YAML::Node>& nodes) {
    int numberOfValidElements = 0;
    for (auto node : nodes) {
        if (extractFromNode(node))
            ++numberOfValidElements;
    }

    return numberOfValidElements;
}

bool ScinthDefManager::extractFromNode(const YAML::Node& node) {
    if (!node.IsMap()) {
        spdlog::error("Top-level yaml node is not a map.");
        return false;
    }

    // Two currently required tags are "name" and "vgens", check they exist.
    if (!node["name"]) {
        spdlog::error("Missing ScinthDef name tag.");
        return false;
    }

    std::string name = node["name"].as<std::string>();

    if (!node["vgens"] || !node["vgens"].IsSequence()) {
        spdlog::error("ScinthDef named {} missing or not sequence vgens key", name);
        return false;
    }

    std::vector<VGen> instances;
    for (auto vgen : node["vgens"]) {
        // className, rate, inputs (can be optional)
        if (!vgen.IsMap()) {
            spdlog::error("ScinthDef {} has vgen that is not a map.", name);
            return false;
        }
        if (!vgen["className"]) {
            spdlog::error("ScinthDef {} has vgen with no className key.", name);
            return false;
        }
        std::string className = vgen["className"].as<std::string>();
        std::shared_ptr<const AbstractVGen> vgenClass = m_vgenManager->getVGenNamed(className);
        if (!vgenClass) {
            spdlog::error("ScinthDef {} has vgen with VGen {} not defined.", name, className);
            return false;
        }

        // TODO: parse rate key

        VGen instance(vgenClass);
        if (vgen["inputs"]) {
            for (auto input : vgen["inputs"]) {
                if (!input.IsMap()) {
                    spdlog::error("ScinthDef {} has VGen {} with non-map input.", name, className);
                    return false;
                }
                if (!input["type"]) {
                    spdlog::error("ScinthDef {} has VGen {} with no type key.", name, className);
                    return false;
                }
                std::string inputType = input["type"].as<std::string>();
                if (inputType == "constant") {
                    if (!input["value"]) {
                        spdlog::error("ScinthDef {} has VGen {} constant input with no value key.", name, className);
                        return false;
                    }
                    float constantValue = input["value"].as<float>();
                    instance.addConstantInput(constantValue);
                } else if (inputType == "vgen") {
                    if (!input["vgenIndex"]) {
                        spdlog::error("ScinthDef {} has VGen {} vgen input with no vgenIndex key.", name, className);
                        return false;
                    }
                    int index = input["vgenIndex"].as<int>();
                    if (index < 0 || index > instances.size()) {
                        spdlog::error("ScinthDef {} has VGen {} vgen input with invalid index {}.", name, className,
                                      index);
                        return false;
                    }
                    instance.addVGenInput(index);
                } else {
                    spdlog::error("ScinthDef {} has VGen {} with undefined input type {}.", name, className, inputType);
                    return false;
                }
            }
        }

        if (!instance.validate()) {
            spdlog::error("ScinthDef {} has invalid VGen {}.", name, className);
            return false;
        }

        instances.push_back(instance);
    }

    std::shared_ptr<ScinthDef> scinthDef(new ScinthDef(instances));
    if (!scinthDef->buildShaders(false)) {
        spdlog::error("ScinthDef {} failed to build shaders.", name);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_scinthDefs.insert_or_assign(name, scinthDef);
    }
    spdlog::info("ScinthDef {} parsed, validated, and added to manager.", name);
    return true;
}

} // namespace scin
