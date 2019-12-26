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
                    if (!input["type"] || !input["type"].IsScalar()) {
                        spdlog::error("ScinthDef {} has VGen {} input with absent or malformed type key.", name,
                                      className);
                        parseError = true;
                        break;
                    }
                    std::string inputType = input["type"].as<std::string>();
                    if (!input["dimension"] || !input["dimension"].IsScalar()) {
                        spdlog::error("ScinthDef {} has VGen {} input with absent or malformed dimension key.", name,
                                      className);
                        parseError = true;
                        break;
                    }
                    int dimension = input["dimension"].as<int>();
                    if (inputType == "constant") {
                        if (!input["value"]) {
                            spdlog::error("ScinthDef {} has VGen {} constant input with no value key.", name,
                                          className);
                            parseError = true;
                            break;
                        } // TODO: higher-dimensional constants.
                        float constantValue = input["value"].as<float>();
                        instance.addConstantInput(constantValue);
                    } else if (inputType == "vgen") {
                        if (!input["vgenIndex"] || !input["outputIndex"]) {
                            spdlog::error("ScinthDef {} has VGen {} vgen input with no vgenIndex or outputIndex key.",
                                          name, className);
                            parseError = true;
                            break;
                        }
                        int vgenIndex = input["vgenIndex"].as<int>();
                        if (vgenIndex < 0 || vgenIndex > instances.size()) {
                            spdlog::error("ScinthDef {} has VGen {} vgen input with invalid index {}.", name, className,
                                          vgenIndex);
                            parseError = true;
                            break;
                        }
                        int outputIndex = input["outputIndex"].as<int>();
                        if (outputIndex < 0 || outputIndex >= instances[vgenIndex].abstractVGen()->outputs().size()) {
                            spdlog::error("ScinthDef {} has VGen {} vgen input with invalid output index {}.", name,
                                          className, outputIndex);
                            parseError = true;
                            break;
                        }
                        instance.addVGenInput(vgenIndex, outputIndex, dimension);
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

        std::shared_ptr<AbstractScinthDef> scinthDef(new AbstractScinthDef(name, instances));
        if (!scinthDef->build()) {
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
        // Required tags are "name", "outputs", "dimension", and "shader".
        if (!node["name"] || !node["name"].IsScalar()) {
            spdlog::error("VGen name tag either absent or not a scalar.");
            continue;
        }
        std::string name = node["name"].as<std::string>();

        if (!node["outputs"] || !node["outputs"].IsSequence()) {
            spdlog::error("VGen {} output tag either absent or not a sequence.", name);
            continue;
        }
        std::vector<std::string> outputs;
        for (auto output : node["outputs"]) {
            outputs.push_back(output.as<std::string>());
        }
        if (outputs.size() == 0) {
            spdlog::error("VGen {} has no outputs.", name);
            continue;
        }

        if (!node["shader"] || !node["shader"].IsScalar()) {
            spdlog::error("VGen {} shader tag absent or not a scalar.", name);
            continue;
        }
        std::string shader = node["shader"].as<std::string>();

        // Check for inputs (optional) first, to get an input count to validate dimension data.
        std::vector<std::string> inputs;
        if (node["inputs"] && node["inputs"].IsSequence()) {
            for (auto input : node["inputs"]) {
                inputs.push_back(input.as<std::string>());
            }
        }

        if (!node["dimension"] || !node["dimension"].IsSequence()) {
            spdlog::error("VGen {} dimension tag absent or not a sequence.", name);
            continue;
        }
        std::vector<std::vector<int>> inputDimensions;
        std::vector<std::vector<int>> outputDimensions;
        for (auto dim : node["dimension"]) {
            if (!dim.IsMap()) {
                spdlog::error("VGen {} has dimension list element that is not a map.", name);
                break;
            }

            // The input tag is optional for those VGens that don't have inputs.
            std::vector<int> inputDims;
            if (dim["inputs"]) {
                // If only a single number provided use that as dimension for all inputs.
                if (dim["inputs"].IsScalar()) {
                    inputDims.insert(inputDims.begin(), inputs.size(), dim["inputs"].as<int>());
                } else if (dim["inputs"].IsSequence()) {
                    for (auto inDim : dim["inputs"]) {
                        inputDims.push_back(inDim.as<int>());
                    }
                } else {
                    spdlog::error("VGen {} has malformed inputs tag inside of dimension list.", name);
                    break;
                }
            }
            inputDimensions.push_back(inputDims);

            // The output tag is required, for all VGens have outputs.
            if (!dim["outputs"]) {
                spdlog::error("VGen {} missing output tag inside of dimension list.", name);
                break;
            }
            std::vector<int> outputDims;
            if (dim["outputs"].IsScalar()) {
                outputDims.insert(outputDims.begin(), outputs.size(), dim["outputs"].as<int>());
            } else if (dim["outputs"].IsSequence()) {
                for (auto outDim : dim["outputs"]) {
                    outputDims.push_back(outDim.as<int>());
                }
            } else {
                spdlog::error("VGen {} has malformed outputs tag inside of dimension list.", name);
                break;
            }
            outputDimensions.push_back(outputDims);
        }

        // Validate output and input dimensions as parsed. There should be at least one entry and the same number of
        // entries in both output and input dimensions arrays.
        if (outputDimensions.size() != inputDimensions.size() || outputDimensions.size() == 0) {
            spdlog::error("VGen {} has mismatched or empty dimensions lists.", name);
            continue;
        }
        // Each entry in input dimensions list should match the number of inputs, and same with outputs.
        bool dimensionsValid = true;
        for (auto i = 0; i < outputDimensions.size(); ++i) {
            if (outputDimensions[i].size() != outputs.size()) {
                spdlog::error("VGen {} has output dimensions list of unequal size to the number of outputs.", name);
                dimensionsValid = false;
                break;
            }
            if (inputDimensions[i].size() != inputs.size()) {
                spdlog::error("VGen {} has input dimensions list of unequal size to the number of inputs.", name);
                dimensionsValid = false;
                break;
            }
        }
        if (!dimensionsValid) {
            continue;
        }

        std::vector<std::string> intermediates;
        if (node["intermediates"] && node["intermediates"].IsSequence()) {
            for (auto intermediate : node["intermediates"]) {
                intermediates.push_back(intermediate.as<std::string>());
            }
        }

        std::shared_ptr<AbstractVGen> vgen(
            new AbstractVGen(name, inputs, outputs, inputDimensions, outputDimensions, shader));
        if (!vgen->prepareTemplate()) {
            spdlog::error("VGen {} failed template preparation.", name);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_vgensMutex);
            m_abstractVGens.insert_or_assign(name, vgen);
        }
        ++numberOfValidElements;
    }

    return numberOfValidElements;
}


} // namespace scin
