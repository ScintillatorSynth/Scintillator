#include "core/ScinthDefParser.hpp"

#include "core/AbstractScinthDef.hpp"
#include "core/AbstractVGen.hpp"
#include "core/VGen.hpp"

#include "spdlog/spdlog.h"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/yaml.h"

namespace scin {

ScinthDefParser::ScinthDefParser() {}
ScinthDefParser::~ScinthDefParser() {}

std::vector<std::shared_ptr<const AbstractScinthDef>> ScinthDefParser::loadFromFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes = parseYAMLFile(fileName);
    return extractFromNodes(nodes);
}

std::vector<std::shared_ptr<const AbstractScinthDef>> ScinthDefParser::parseFromString(const std::string& yaml) {
    std::vector<YAML::Node> nodes = parseYAMLString(yaml);
    return extractFromNodes(nodes);
}

std::shared_ptr<const AbstractScinthDef> ScinthDefParser::getAbstractScinthDefNamed(const std::string& name) {
    std::shared_ptr<const AbstractScinthDef> scinthDef;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_scinthDefs.find(name);
    if (it != m_scinthDefs.end()) {
        scinthDef = it->second;
    }
    return scinthDef;
}

size_t ScinthDefParser::numberOfAbstractScinthDefs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_scinthDefs.size();
}

int ScinthDefParser::loadAbstractVGensFromFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes = parseYAMLFile(fileName);
    return extractAbstractVGensFromNodes(nodes);
}

int ScinthDefParser::parseAbstractVGensFromString(const std::string& yaml) {
    std::vector<YAML::Node> nodes = parseYAMLString(yaml);
    return extractAbstractVGensFromNodes(nodes);
}

std::shared_ptr<const AbstractVGen> ScinthDefParser::getAbstractVGenNamed(const std::string& name) {
    std::shared_ptr<const AbstractVGen> vgen;
    std::lock_guard<std::mutex> lock(m_vgensMutex);
    auto it = m_abstractVGens.find(name);
    if (it != m_abstractVGens.end()) {
        vgen = it->second;
    }
    return vgen;
}

size_t ScinthDefParser::numberOfAbstractVGens() {
    std::lock_guard<std::mutex> lock(m_vgensMutex);
    return m_abstractVGens.size();
}

std::vector<YAML::Node> ScinthDefParser::parseYAMLFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAllFromFile(fileName);
    } catch (const YAML::ParserException&) {
        spdlog::error("error parsing yaml file {}", fileName);
        return std::vector<YAML::Node>();
    } catch (const YAML::BadFile&) {
        spdlog::error("bad yaml file {}", fileName);
        return std::vector<YAML::Node>();
    }
    return nodes;
}

std::vector<YAML::Node> ScinthDefParser::parseYAMLString(const std::string& yaml) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAll(yaml);
    } catch (const YAML::ParserException&) {
        spdlog::error("error parsing yaml string {}", yaml);
        return std::vector<YAML::Node>();
    }
    return nodes;
}


std::vector<std::shared_ptr<const AbstractScinthDef>>
ScinthDefParser::extractFromNodes(const std::vector<YAML::Node>& nodes) {
    std::vector<std::shared_ptr<const AbstractScinthDef>> scinthDefs;
    for (auto node : nodes) {
        if (!node.IsMap()) {
            spdlog::error("Top-level yaml node is not a map.");
            continue;
        }

        // Two currently required tags are "name" and "vgens", check they exist.
        if (!node["name"]) {
            spdlog::error("Missing ScinthDef name tag.");
            continue;
        }

        std::string name = node["name"].as<std::string>();

        if (!node["vgens"] || !node["vgens"].IsSequence()) {
            spdlog::error("ScinthDef named {} missing or not sequence vgens key", name);
            continue;
        }

        bool parseError = false;
        std::vector<VGen> instances;
        for (auto vgen : node["vgens"]) {
            // className, rate, inputs (can be optional)
            if (!vgen.IsMap()) {
                spdlog::error("ScinthDef {} has vgen that is not a map.", name);
                parseError = true;
                break;
            }
            if (!vgen["className"]) {
                spdlog::error("ScinthDef {} has vgen with no className key.", name);
                parseError = true;
                break;
            }
            std::string className = vgen["className"].as<std::string>();
            std::shared_ptr<const AbstractVGen> vgenClass = getAbstractVGenNamed(className);
            if (!vgenClass) {
                spdlog::error("ScinthDef {} has vgen with VGen {} not defined.", name, className);
                parseError = true;
                break;
            }

            // TODO: parse rate key

            VGen instance(vgenClass);
            if (!parseError && vgen["inputs"]) {
                for (auto input : vgen["inputs"]) {
                    if (!input.IsMap()) {
                        spdlog::error("ScinthDef {} has VGen {} with non-map input.", name, className);
                        parseError = true;
                        break;
                    }
                    if (!input["type"]) {
                        spdlog::error("ScinthDef {} has VGen {} with no type key.", name, className);
                        parseError = true;
                        break;
                    }
                    std::string inputType = input["type"].as<std::string>();
                    if (inputType == "constant") {
                        if (!input["value"]) {
                            spdlog::error("ScinthDef {} has VGen {} constant input with no value key.", name,
                                          className);
                            parseError = true;
                            break;
                        }
                        float constantValue = input["value"].as<float>();
                        instance.addConstantInput(constantValue);
                    } else if (inputType == "vgen") {
                        if (!input["vgenIndex"]) {
                            spdlog::error("ScinthDef {} has VGen {} vgen input with no vgenIndex key.", name,
                                          className);
                            parseError = true;
                            break;
                        }
                        int index = input["vgenIndex"].as<int>();
                        if (index < 0 || index > instances.size()) {
                            spdlog::error("ScinthDef {} has VGen {} vgen input with invalid index {}.", name, className,
                                          index);
                            parseError = true;
                            break;
                        }
                        instance.addVGenInput(index);
                    } else {
                        spdlog::error("ScinthDef {} has VGen {} with undefined input type {}.", name, className,
                                      inputType);
                        parseError = true;
                        break;
                    }
                }
            }

            if (!instance.validate()) {
                spdlog::error("ScinthDef {} has invalid VGen {}.", name, className);
                parseError = true;
                break;
            }

            instances.push_back(instance);
        }

        if (parseError)
            continue;

        std::shared_ptr<AbstractScinthDef> scinthDef(new AbstractScinthDef(instances));
        if (!scinthDef->buildShaders(false)) {
            spdlog::error("ScinthDef {} failed to build shaders.", name);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_scinthDefs.insert_or_assign(name, scinthDef);
        }
        spdlog::info("ScinthDef {} parsed, validated, and added to manager.", name);
        scinthDefs.push_back(scinthDef);
    }

    return scinthDefs;
}

int ScinthDefParser::extractAbstractVGensFromNodes(const std::vector<YAML::Node>& nodes) {
    int numberOfValidElements = 0;
    for (auto node : nodes) {
        // Top level structure expected is a Map.
        if (!node.IsMap()) {
            spdlog::error("Top-level abstract VGen yaml node is not a map.");
            continue;
        }
        // Two currently required tags are "name" and "fragment", check they exist.
        if (!node["name"] || !node["fragment"]) {
            spdlog::error("Missing either name or fragment tag.");
            continue;
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
            std::lock_guard<std::mutex> lock(m_vgensMutex);
            m_abstractVGens.insert_or_assign(name, vgen);
        }
        ++numberOfValidElements;
    }

    return numberOfValidElements;
}


} // namespace scin
