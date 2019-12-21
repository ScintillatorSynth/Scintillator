#include "ScinthDefManager.hpp"

#include "VGenInstance.hpp"
#include "VGenManager.hpp"

#include "spdlog/spdlog.h"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/yaml.h"

namespace scin {

ScinthDefManager::ScinthDefManager(std::shared_ptr<VGenManager> vgenManager): m_vgenManager(vgenManager) {}

ScinthDefManager::~ScinthDefManager() {}

int ScinthDefManager::loadFromFile(const std::string& fileName) { return 0; }

int ScinthDefManager::parseFromString(const std::string& yaml) { return 0; }

int ScinthDefManager::extractFromNodes(const std::vector<YAML::Node>& nodes) { return 0; }

bool ScinthDefManager::extractFromNode(const YAML::Node& node) {
    if (!node.IsMap()) {
        spdlog::warn("Top-level yaml node is not a map.");
        return false;
    }

    // Two currently required tags are "name" and "vgens", check they exist.
    if (!node["name"]) {
        spdlog::warn("Missing ScinthDef name tag.");
        return false;
    }

    std::string name = node["name"].as<std::string>();

    if (!node["vgens"] || !node["vgens"].IsSequence()) {
        spdlog::warn("ScinthDef named {} missing or not sequence vgens key", name);
        return false;
    }

    std::vector<VGenInstance> instances;
    for (auto vgen : node["vgens"]) {
        // className, rate, inputs (can be optional)
        if (!vgen.IsMap()) {
            spdlog::warn("ScinthDef {} has vgen that is not a map.", name);
            return false;
        }
        if (!node["className"]) {
            spdlog::warn("ScinthDef {} has vgen with no className key.", name);
            return false;
        }
        std::string className = node["className"].as<std::string>();
        std::shared_ptr<VGen> vgenClass = m_vgenManager->getVGenNamed(className);
        if (!vgenClass) {
            spdlog::warn("ScinthDef {} has vgen with VGen {} not defined.", name, className);
        }

        // TODO: parse rate key

        VGenInstance instance(vgenClass);
        if (node["inputs"]) {
            for (auto input : node["inputs"]) {
                if (!input.IsMap()) {
                    spdlog::warn("ScinthDef {} has VGen {} with non-map input.", name, className);
                    return false;
                }
                if (!input["type"]) {
                    spdlog::warn("ScinthDef {} has VGen {} with no type key.", name, className);
                    return false;
                }
                std::string inputType = input["type"].as<std::string>();
                if (inputType == "constant") {
                    if (!input["value"]) {
                        spdlog::warn("ScinthDef {} has VGen {} constant input with no value key.", name, className);
                        return false;
                    }
                    float constantValue = input["value"].as<float>();
                    instance.addConstantInput(constantValue);
                } else if (inputType == "vgen") {
                    if (!input["vgenIndex"]) {
                        spdlog::warn("ScinthDef {} has VGen {} vgen input with no vgenIndex key.", name, className);
                        return false;
                    }
                    int index = input["vgenIndex"].as<int>();
                    if (index < 0 || index > instances.size()) {
                        spdlog::warn("ScinthDef {} has VGen {} vgen input with invalid index {}.", name, className,
                                     index);
                        return false;
                    }
                    instance.addVGenInput(index);
                } else {
                    spdlog::warn("ScinthDef {} has VGen {} with undefined input type {}.", name, className, inputType);
                    return false;
                }
            }
        }

        if (!instance.validate()) {
            spdlog::warn("ScinthDef {} has invalid VGen {}.", name, className);
            return false;
        }

        instances.push_back(instance);
    }

    return true;
}

} // namespace scin
