#include "base/Archetypes.hpp"

#include "base/AbstractSampler.hpp"
#include "base/AbstractScinthDef.hpp"
#include "base/AbstractVGen.hpp"
#include "base/Parameter.hpp"
#include "base/VGen.hpp"

#include "spdlog/spdlog.h"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/yaml.h"

namespace scin { namespace base {

Archetypes::Archetypes() {
    m_vgenInputTypes.insert({ "constant", VGen::InputType::kConstant });
    m_vgenInputTypes.insert({ "vgen", VGen::InputType::kVGen });
    m_vgenInputTypes.insert({ "parameter", VGen::InputType::kParameter });

    m_samplerFilterModes.insert({ "linear", AbstractSampler::FilterMode::kLinear });
    m_samplerFilterModes.insert({ "nearest", AbstractSampler::FilterMode::kNearest });

    m_samplerAddressModes.insert({ "clampToBorder", AbstractSampler::AddressMode::kClampToBorder });
    m_samplerAddressModes.insert({ "clampToEdge", AbstractSampler::AddressMode::kClampToEdge });
    m_samplerAddressModes.insert({ "repeat", AbstractSampler::AddressMode::kRepeat });
    m_samplerAddressModes.insert({ "mirroredRepeat", AbstractSampler::AddressMode::kMirroredRepeat });

    m_samplerBorderColors.insert({ "transparentBlack", AbstractSampler::ClampBorderColor::kTransparentBlack });
    m_samplerBorderColors.insert({ "black", AbstractSampler::ClampBorderColor::kBlack });
    m_samplerBorderColors.insert({ "white", AbstractSampler::ClampBorderColor::kWhite });
}

Archetypes::~Archetypes() {}

std::vector<std::shared_ptr<const AbstractScinthDef>> Archetypes::loadFromFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes = parseYAMLFile(fileName);
    return extractFromNodes(nodes);
}

std::vector<std::shared_ptr<const AbstractScinthDef>> Archetypes::parseFromString(const std::string& yaml) {
    std::vector<YAML::Node> nodes = parseYAMLString(yaml);
    return extractFromNodes(nodes);
}

std::shared_ptr<const AbstractScinthDef> Archetypes::getAbstractScinthDefNamed(const std::string& name) {
    std::shared_ptr<const AbstractScinthDef> scinthDef;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_scinthDefs.find(name);
    if (it != m_scinthDefs.end()) {
        scinthDef = it->second;
    }
    return scinthDef;
}

void Archetypes::freeAbstractScinthDefs(const std::vector<std::string>& names) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto name : names) {
        auto it = m_scinthDefs.find(name);
        if (it != m_scinthDefs.end()) {
            m_scinthDefs.erase(it);
        } else {
            spdlog::warn("failed to free AbstractScinthDef {}, name not found", name);
        }
    }
}

size_t Archetypes::numberOfAbstractScinthDefs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_scinthDefs.size();
}

int Archetypes::loadAbstractVGensFromFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes = parseYAMLFile(fileName);
    return extractAbstractVGensFromNodes(nodes);
}

int Archetypes::parseAbstractVGensFromString(const std::string& yaml) {
    std::vector<YAML::Node> nodes = parseYAMLString(yaml);
    return extractAbstractVGensFromNodes(nodes);
}

std::shared_ptr<const AbstractVGen> Archetypes::getAbstractVGenNamed(const std::string& name) {
    std::shared_ptr<const AbstractVGen> vgen;
    std::lock_guard<std::mutex> lock(m_vgensMutex);
    auto it = m_abstractVGens.find(name);
    if (it != m_abstractVGens.end()) {
        vgen = it->second;
    }
    return vgen;
}

size_t Archetypes::numberOfAbstractVGens() {
    std::lock_guard<std::mutex> lock(m_vgensMutex);
    return m_abstractVGens.size();
}

std::vector<YAML::Node> Archetypes::parseYAMLFile(const std::string& fileName) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAllFromFile(fileName);
    } catch (const YAML::ParserException& e) {
        spdlog::error("error parsing yaml file {}: {}", fileName, e.what());
        return std::vector<YAML::Node>();
    } catch (const YAML::BadFile& e) {
        spdlog::error("bad yaml file {}: {}", fileName, e.what());
        return std::vector<YAML::Node>();
    } catch (const std::runtime_error& e) {
        spdlog::error("stdlib exception during YAML parsing of {}:", fileName, e.what());
        return std::vector<YAML::Node>();
    }
    return nodes;
}

std::vector<YAML::Node> Archetypes::parseYAMLString(const std::string& yaml) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAll(yaml);
    } catch (const YAML::ParserException& e) {
        spdlog::error("error parsing yaml string {}: {}", yaml, e.what());
        return std::vector<YAML::Node>();
    }
    return nodes;
}

std::vector<std::shared_ptr<const AbstractScinthDef>>
Archetypes::extractFromNodes(const std::vector<YAML::Node>& nodes) {
    std::vector<std::shared_ptr<const AbstractScinthDef>> scinthDefs;
    for (auto node : nodes) {
        if (!node.IsMap()) {
            spdlog::error("Top-level yaml node is not a map.");
            continue;
        }

        // Parse name of ScinthDef.
        if (!node["name"]) {
            spdlog::error("Missing ScinthDef name tag.");
            continue;
        }

        std::string name = node["name"].as<std::string>();

        // Parse parameter list if present.
        bool parseError = false;
        std::vector<Parameter> parameters;
        if (node["parameters"]) {
            if (!node["parameters"].IsSequence()) {
                spdlog::error("ScinthDef named {} got non-sequence parameters key", name);
                continue;
            }

            for (auto param : node["parameters"]) {
                // Expecting a dictionary with two keys per entry, "name" and "defaultValue".
                if (!param.IsMap()) {
                    spdlog::error("ScinthDef {} got non-dictionary parameters entry.", name);
                    parseError = true;
                    break;
                }

                if (!param["name"] || !param["defaultValue"]) {
                    spdlog::error("ScinthDef {} has parameters entry missing requires key.", name);
                    parseError = true;
                    break;
                }

                parameters.emplace_back(Parameter(param["name"].as<std::string>(), param["defaultValue"].as<float>()));
            }

            if (parseError) {
                continue;
            }
        }

        // Parse VGen list.
        if (!node["vgens"] || !node["vgens"].IsSequence()) {
            spdlog::error("ScinthDef named {} missing or not sequence vgens key", name);
            continue;
        }

        std::vector<VGen> instances;
        for (auto vgen : node["vgens"]) {
            // className, rate, outputs, inputs (can be optional)
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
                spdlog::error("ScinthDef {} has vgen with className {} not defined.", name, className);
                parseError = true;
                break;
            }

            // TODO: parse rate key

            VGen instance(vgenClass);

            if (vgen["sampler"] && vgen["sampler"].IsMap()) {
                if (!vgenClass->isSampler()) {
                    spdlog::error("ScinthDef {} has non-sampler VGen {} with sampler dictionary", name, className);
                    parseError = true;
                    break;
                }

                auto sampler = vgen["sampler"];
                // Two required keys "image" and "imageArgType".
                if (!sampler["image"] || !sampler["imageArgType"]) {
                    spdlog::error("ScinthDef {} has sampler VGen {} missing image or imageArgType keys.", name,
                                  className);
                    parseError = true;
                    break;
                }

                int imageIndex = sampler["image"].as<int>();
                std::string imageArgTypeString = sampler["imageArgType"].as<std::string>();
                auto imageArgIt = m_vgenInputTypes.find(imageArgTypeString);
                if (imageArgIt == m_vgenInputTypes.end()) {
                    spdlog::error("ScinthDef {} has sampler VGen {} with invalid image arg type string {}.", name,
                                  className, imageArgTypeString);
                    parseError = true;
                    break;
                }

                VGen::InputType imageArgType = imageArgIt->second;
                if (imageArgType == VGen::InputType::kVGen) {
                    spdlog::error("ScinthDef {} has sampler VGen {} with VGen input type.", name, className);
                    parseError = true;
                    break;
                }

                AbstractSampler samplerConfig;
                if (sampler["minFilterMode"]) {
                    auto it = m_samplerFilterModes.find(sampler["minFilterMode"].as<std::string>());
                    if (it == m_samplerFilterModes.end()) {
                        spdlog::error("ScinthDef {} has sampler VGen {} with unsupported minFilterMode {}", name,
                                      className, sampler["minFilterMode"].as<std::string>());
                        parseError = true;
                        break;
                    }
                    samplerConfig.setMinFilterMode(it->second);
                }

                if (sampler["magFilterMode"]) {
                    auto it = m_samplerFilterModes.find(sampler["magFilterMode"].as<std::string>());
                    if (it == m_samplerFilterModes.end()) {
                        spdlog::error("ScinthDef {} has sampler VGen {} with unsupported magFilterMode {}", name,
                                      className, sampler["magFilterMode"].as<std::string>());
                        parseError = true;
                        break;
                    }
                    samplerConfig.setMagFilterMode(it->second);
                }

                if (sampler["enableAnisotropicFiltering"]) {
                    samplerConfig.enableAnisotropicFiltering(sampler["enableAnisotropicFiltering"].as<bool>());
                }

                if (sampler["addressModeU"]) {
                    auto it = m_samplerAddressModes.find(sampler["addressModeU"].as<std::string>());
                    if (it == m_samplerAddressModes.end()) {
                        spdlog::error("ScinthDef {} has sampler VGen {} with unsupported addressModeU {}", name,
                                      className, sampler["addressModeU"].as<std::string>());
                        parseError = true;
                        break;
                    }
                    samplerConfig.setAddressModeU(it->second);
                }

                if (sampler["addressModeV"]) {
                    auto it = m_samplerAddressModes.find(sampler["addressModeV"].as<std::string>());
                    if (it == m_samplerAddressModes.end()) {
                        spdlog::error("ScinthDef {} has sampler VGen {} with unsupported addressModeV {}", name,
                                      className, sampler["addressModeV"].as<std::string>());
                        parseError = true;
                        break;
                    }
                    samplerConfig.setAddressModeV(it->second);
                }

                if (sampler["clampBorderColor"]) {
                    auto it = m_samplerBorderColors.find(sampler["clampBorderColor"].as<std::string>());
                    if (it == m_samplerBorderColors.end()) {
                        spdlog::error("ScinthDef {} has sampler VGen {} with unsupported clampBorderColor {}", name,
                                      className, sampler["clampBorderColor"].as<std::string>());
                        parseError = true;
                        break;
                    }
                    samplerConfig.setClampBorderColor(it->second);
                }

                instance.setSamplerConfig(imageIndex, imageArgType, samplerConfig);
            } else {
                if (vgenClass->isSampler()) {
                    spdlog::error("ScinthDef {} has sampler VGen {} with no sampler dictionary", name, className);
                    parseError = true;
                    break;
                }
            }

            if (!vgen["outputs"] || !vgen["outputs"].IsSequence()) {
                spdlog::error("ScinthDef {} has vgen with className {} with absent or malformed outputs key", name,
                              className);
                parseError = true;
                break;
            }
            for (auto output : vgen["outputs"]) {
                if (!output.IsMap()) {
                    spdlog::error("ScinthDef {} has VGen {} with non-map output.", name, className);
                    parseError = true;
                    break;
                }
                if (!output["dimension"] || !output["dimension"].IsScalar()) {
                    spdlog::error("ScinthDef {} has VGen {} with absent or malformed dimension key.", name, className);
                    parseError = true;
                    break;
                }
                instance.addOutput(output["dimension"].as<int>());
            }

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

                    std::string inputTypeString = input["type"].as<std::string>();
                    auto typeIt = m_vgenInputTypes.find(inputTypeString);
                    if (typeIt == m_vgenInputTypes.end()) {
                        spdlog::error("ScinthDef {} has VGen {} with undefined input type {}.", name, className,
                                      inputTypeString);
                        parseError = true;
                        break;
                    }
                    VGen::InputType inputType = typeIt->second;

                    if (!input["dimension"] || !input["dimension"].IsScalar()) {
                        spdlog::error("ScinthDef {} has VGen {} input with absent or malformed dimension key.", name,
                                      className);
                        parseError = true;
                        break;
                    }
                    int dimension = input["dimension"].as<int>();
                    if (inputType == VGen::InputType::kConstant) {
                        if (!input["value"]) {
                            spdlog::error("ScinthDef {} has VGen {} constant input with no value key.", name,
                                          className);
                            parseError = true;
                            break;
                        }
                        // TODO: higher-dimensional constants.
                        float constantValue = input["value"].as<float>();
                        instance.addConstantInput(constantValue);
                    } else if (inputType == VGen::InputType::kVGen) {
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
                    } else if (inputType == VGen::InputType::kParameter) {
                        if (!input["index"]) {
                            spdlog::error("ScinthDef {} has VGen {} parameter input with no index key.", name,
                                          className);
                            parseError = true;
                            break;
                        }
                        int parameterIndex = input["index"].as<int>();
                        instance.addParameterInput(parameterIndex);
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

        std::shared_ptr<AbstractScinthDef> scinthDef(new AbstractScinthDef(name, parameters, instances));
        if (!scinthDef->build()) {
            spdlog::error("ScinthDef {} failed to build shaders.", name);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_scinthDefs.insert_or_assign(name, scinthDef);
        }
        spdlog::info("ScinthDef {} parsed, validated, and added to Archetypes.", name);
        scinthDefs.push_back(scinthDef);
    }

    return scinthDefs;
}

int Archetypes::extractAbstractVGensFromNodes(const std::vector<YAML::Node>& nodes) {
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

        unsigned supportedRates = 0;
        if (!node["rates"] || !node["rates"].IsSequence()) {
            spdlog::error("VGen rates tag either absent or not a list.");
            continue;
        }
        for (auto rateNode : node["rates"]) {
            std::string rate = rateNode.as<std::string>();
            if (rate == "frame") {
                supportedRates |= AbstractVGen::Rates::kFrame;
            } else if (rate == "shape") {
                supportedRates |= AbstractVGen::Rates::kShape;
            } else if (rate == "pixel") {
                supportedRates |= AbstractVGen::Rates::kPixel;
            } else {
                spdlog::error("VGen {} has unsupported rate tag {}.", name, rate);
                supportedRates = 0;
                break;
            }
        }
        if (supportedRates == 0) {
            spdlog::error("VGen {} has problem with rate configuration.", name);
            continue;
        }

        bool isSampler = false;
        if (node["sampler"] && node["sampler"].IsScalar()) {
            isSampler = node["sampler"].as<bool>();
        }

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

        if (!node["dimensions"] || !node["dimensions"].IsSequence()) {
            spdlog::error("VGen {} dimensions tag absent or not a sequence.", name);
            continue;
        }
        std::vector<std::vector<int>> inputDimensions;
        std::vector<std::vector<int>> outputDimensions;
        for (auto dim : node["dimensions"]) {
            if (!dim.IsMap()) {
                spdlog::error("VGen {} has dimensions list element that is not a map.", name);
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

        std::shared_ptr<AbstractVGen> vgen(new AbstractVGen(name, supportedRates, isSampler, inputs, outputs,
                                                            inputDimensions, outputDimensions, shader));
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

} // namespace base

} // namespace scin
