#include "base/Archetypes.hpp"

#include "base/AbstractSampler.hpp"
#include "base/AbstractScinthDef.hpp"
#include "base/AbstractTween.hpp"
#include "base/AbstractVGen.hpp"
#include "base/Parameter.hpp"
#include "base/RenderOptions.hpp"
#include "base/Shape.hpp"
#include "base/VGen.hpp"

#include "spdlog/spdlog.h"

#if _MSC_VER
// Disable MSVC warning on included yaml-cpp headers
#    pragma warning(push)
#    pragma warning(disable : 4996)
#endif // _MSC_VER

#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/yaml.h"

#if _MSC_VER
#    pragma warning(pop)
#endif // _MSC_VER

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

std::shared_ptr<AbstractScinthDef> Archetypes::parseOnly(const std::string& yaml) {
    std::vector<YAML::Node> nodes;
    try {
        nodes = YAML::LoadAll(yaml);
    } catch (const YAML::ParserException& e) {
        spdlog::error("error parsing yaml string {}: {}", yaml, e.what());
        return nullptr;
    }
    if (nodes.size() != 1) {
        spdlog::error("expected 1 node in AbstractScinthDef yaml, got {}", nodes.size());
        return nullptr;
    }

    return extractSingleNode(nodes[0]);
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
    for (const auto& node : nodes) {
        std::shared_ptr<AbstractScinthDef> scinthDef = extractSingleNode(node);
        if (!scinthDef) {
            continue;
        }

        if (!scinthDef->build()) {
            spdlog::error("ScinthDef {} failed to build shaders.", scinthDef->name());
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_scinthDefs.insert_or_assign(scinthDef->name(), scinthDef);
        }
        spdlog::info("ScinthDef {} parsed, validated, and added to Archetypes.", scinthDef->name());
        scinthDefs.push_back(scinthDef);
    }

    return scinthDefs;
}

std::shared_ptr<AbstractScinthDef> Archetypes::extractSingleNode(const YAML::Node& node) {
    if (!node.IsMap()) {
        spdlog::error("Top-level yaml node is not a map.");
        return nullptr;
    }

    // Parse name of ScinthDef.
    if (!node["name"]) {
        spdlog::error("Missing ScinthDef name tag.");
        return nullptr;
    }

    std::string name = node["name"].as<std::string>();

    // Parse shape.
    if (!node["shape"] || !node["shape"].IsMap()) {
        spdlog::error("ScinthDef {} absent or malformed shape tag.");
        return nullptr;
    }

    std::unique_ptr<Shape> shape;
    auto shapeNode = node["shape"];
    if (!shapeNode["name"]) {
        spdlog::error("ScinthDef {} shape map missing name.", name);
        return nullptr;
    }
    auto shapeName = shapeNode["name"].as<std::string>();
    if (shapeName == "Quad") {
        uint16_t widthEdges = 1;
        uint16_t heightEdges = 1;
        if (shapeNode["widthEdges"] && shapeNode["widthEdges"].IsScalar()) {
            widthEdges = shapeNode["widthEdges"].as<uint16_t>();
        }
        if (shapeNode["heightEdges"] && shapeNode["heightEdges"].IsScalar()) {
            heightEdges = shapeNode["heightEdges"].as<uint16_t>();
        }
        shape.reset(new Quad(widthEdges, heightEdges));
    } else {
        spdlog::error("ScinthDef {} has unsupported shape name {}", name, shapeName);
        return nullptr;
    }

    // Parse render options, if any.
    RenderOptions renderOptions;
    if (node["options"] && node["options"].IsMap()) {
        auto optionsNode = node["options"];
        if (optionsNode["polygonMode"] && optionsNode["polygonMode"].IsScalar()) {
            std::string mode = optionsNode["polygonMode"].as<std::string>();
            if (mode == "fill") {
                renderOptions.setPolygonMode(RenderOptions::PolygonMode::kFill);
            } else if (mode == "line") {
                renderOptions.setPolygonMode(RenderOptions::PolygonMode::kLine);
            } else if (mode == "point") {
                renderOptions.setPolygonMode(RenderOptions::PolygonMode::kPoint);
            } else {
                spdlog::warn("Ignoring unsupported RenderOptions PolygonMode {}", mode);
            }
        }
    }

    // Parse parameter list if present.
    std::vector<Parameter> parameters;
    if (node["parameters"]) {
        if (!node["parameters"].IsSequence()) {
            spdlog::error("ScinthDef named {} got non-sequence parameters key", name);
            return nullptr;
        }

        for (auto param : node["parameters"]) {
            // Expecting a dictionary with two keys per entry, "name" and "defaultValue".
            if (!param.IsMap()) {
                spdlog::error("ScinthDef {} got non-dictionary parameters entry.", name);
                return nullptr;
            }

            if (!param["name"] || !param["defaultValue"]) {
                spdlog::error("ScinthDef {} has parameters entry missing requires key.", name);
                return nullptr;
            }

            parameters.emplace_back(Parameter(param["name"].as<std::string>(), param["defaultValue"].as<float>()));
        }
    }

    // Parse tween list if present.
    std::vector<AbstractTween> tweens;
    if (node["tweens"]) {
        if (!node["tweens"].IsSequence()) {
            spdlog::error("ScinthDef named {} got non-sequence tweens key", name);
            return nullptr;
        }

        for (auto tween : node["tweens"]) {
            // Expecting a dictionary with keys "levels", "durations", "curves", "sampleRate" and "dimension".
            if (!tween.IsMap()) {
                spdlog::error("ScinthDef {} got non-dictionary tween entry.", name);
                return nullptr;
            }

            if (!tween["dimension"] || !tween["dimension"].IsScalar()) {
                spdlog::error("ScinthDef {} has absent or malformed dimension key within tween.", name);
                return nullptr;
            }
            int dimension = tween["dimension"].as<int>();

            if (!tween["sampleRate"] || !tween["sampleRate"].IsScalar()) {
                spdlog::error("ScinthDef {} has absent or malformed sampleRate key within tween.", name);
                return nullptr;
            }
            float sampleRate = tween["sampleRate"].as<float>();

            if (!tween["curves"]) {
                spdlog::error("ScinthDef {} has absent or malformed curves key within tween.", name);
                return nullptr;
            }
            std::vector<AbstractTween::Curve> curves;
            if (tween["curves"].IsScalar()) {
                curves.emplace_back(static_cast<AbstractTween::Curve>(tween["curves"].as<int>()));
            } else if (tween["curves"].IsSequence()) {
                for (auto curve : tween["curves"]) {
                    curves.emplace_back(static_cast<AbstractTween::Curve>(curve.as<int>()));
                }
            } else {
                spdlog::error("ScinthDef {} has tween with invalid curves key.", name);
                return nullptr;
            }

            float totalTime = 0;
            std::vector<float> durations;
            if (!tween["durations"] || !tween["durations"].IsSequence()) {
                spdlog::error("ScinthDef {} has absent or malformed durations key within tween.", name);
                return nullptr;
            }
            for (auto dur : tween["durations"]) {
                float durFloat = dur.as<float>();
                totalTime += durFloat;
                durations.emplace_back(durFloat);
            }

            bool loop = false;
            if (tween["loop"] && tween["loop"].IsScalar()) {
                loop = tween["loop"].as<bool>();
            }

            std::vector<glm::vec4> levels;
            if (!tween["levels"] || !tween["levels"].IsSequence()) {
                spdlog::error("ScinthDef {} has absent or malformed levels key within tween.", name);
                return nullptr;
            }
            switch (dimension) {
            case 1: {
                for (auto v1 : tween["levels"]) {
                    if (!v1.IsScalar()) {
                        spdlog::error("ScinthDef {} has tween with dimension 1 but non-scalar level.", name);
                        return nullptr;
                    }
                    levels.emplace_back(v1.as<float>(), 0.0, 0.0, 0.0);
                }
            } break;

            case 2: {
                for (auto v2 : tween["levels"]) {
                    if (!v2.IsSequence() || v2.size() != 2) {
                        spdlog::error("ScinthDef {} has tween with dimension 2 but not 2-element level.", name);
                        return nullptr;
                    }
                    levels.emplace_back(v2[0].as<float>(), v2[1].as<float>(), 0.0, 0.0);
                }
            } break;

            case 4: {
                for (auto v4 : tween["levels"]) {
                    if (!v4.IsSequence() || v4.size() != 4) {
                        spdlog::error("ScinthDef {} has tween with dimension 4 but not 4-element level.", name);
                        return nullptr;
                    }
                    levels.emplace_back(v4[0].as<float>(), v4[1].as<float>(), v4[2].as<float>(), v4[3].as<float>());
                }
            } break;

            default:
                spdlog::error("ScinthDef {} has unsupported dimension value {} within tween.", name, dimension);
                return nullptr;
            }

            AbstractTween tn(dimension, sampleRate, totalTime, loop, levels, durations, curves);
            if (!tn.validate()) {
                spdlog::error("ScinthDef {} has invalid tween.", name);
                return nullptr;
            }

            tweens.emplace_back(tn);
        }
    }

    // Parse VGen list.
    if (!node["vgens"] || !node["vgens"].IsSequence()) {
        spdlog::error("ScinthDef named {} missing or not sequence vgens key", name);
        return nullptr;
    }

    std::vector<VGen> instances;
    for (auto vgen : node["vgens"]) {
        // className, rate, outputs, inputs (can be optional)
        if (!vgen.IsMap()) {
            spdlog::error("ScinthDef {} has vgen that is not a map.", name);
            return nullptr;
        }
        if (!vgen["className"]) {
            spdlog::error("ScinthDef {} has vgen with no className key.", name);
            return nullptr;
        }
        std::string className = vgen["className"].as<std::string>();
        std::shared_ptr<const AbstractVGen> vgenClass = getAbstractVGenNamed(className);
        if (!vgenClass) {
            spdlog::error("ScinthDef {} has vgen with className {} not defined.", name, className);
            return nullptr;
        }

        AbstractVGen::Rates rate = AbstractVGen::Rates::kNone;
        if (!vgen["rate"] || !vgen["rate"].IsScalar()) {
            spdlog::error("ScinthDef {} has VGen with className {} absent or malformed rate key.", name, className);
            return nullptr;
        }
        std::string rateName = vgen["rate"].as<std::string>();
        if (rateName == "pixel") {
            rate = AbstractVGen::Rates::kPixel;
        } else if (rateName == "shape") {
            rate = AbstractVGen::Rates::kShape;
        } else if (rateName == "frame") {
            rate = AbstractVGen::Rates::kFrame;
        } else {
            spdlog::error("ScinthDef {} has VGen with className {} with unsupported rate value {}.", name, className,
                          rateName);
            return nullptr;
        }

        VGen instance(vgenClass, rate);

        if (vgen["sampler"] && vgen["sampler"].IsMap()) {
            if (!vgenClass->isSampler()) {
                spdlog::error("ScinthDef {} has non-sampler VGen {} with sampler dictionary", name, className);
                return nullptr;
            }

            auto sampler = vgen["sampler"];
            // Two required keys "image" and "imageArgType".
            if (!sampler["image"] || !sampler["imageArgType"]) {
                spdlog::error("ScinthDef {} has sampler VGen {} missing image or imageArgType keys.", name, className);
                return nullptr;
            }

            int imageIndex = sampler["image"].as<int>();
            std::string imageArgTypeString = sampler["imageArgType"].as<std::string>();
            auto imageArgIt = m_vgenInputTypes.find(imageArgTypeString);
            if (imageArgIt == m_vgenInputTypes.end()) {
                spdlog::error("ScinthDef {} has sampler VGen {} with invalid image arg type string {}.", name,
                              className, imageArgTypeString);
                return nullptr;
            }

            VGen::InputType imageArgType = imageArgIt->second;
            if (imageArgType == VGen::InputType::kVGen) {
                spdlog::error("ScinthDef {} has sampler VGen {} with VGen input type.", name, className);
                return nullptr;
            }

            AbstractSampler samplerConfig;
            if (sampler["minFilterMode"]) {
                auto it = m_samplerFilterModes.find(sampler["minFilterMode"].as<std::string>());
                if (it == m_samplerFilterModes.end()) {
                    spdlog::error("ScinthDef {} has sampler VGen {} with unsupported minFilterMode {}", name, className,
                                  sampler["minFilterMode"].as<std::string>());
                    return nullptr;
                }
                samplerConfig.setMinFilterMode(it->second);
            }

            if (sampler["magFilterMode"]) {
                auto it = m_samplerFilterModes.find(sampler["magFilterMode"].as<std::string>());
                if (it == m_samplerFilterModes.end()) {
                    spdlog::error("ScinthDef {} has sampler VGen {} with unsupported magFilterMode {}", name, className,
                                  sampler["magFilterMode"].as<std::string>());
                    return nullptr;
                }
                samplerConfig.setMagFilterMode(it->second);
            }

            if (sampler["enableAnisotropicFiltering"]) {
                samplerConfig.enableAnisotropicFiltering(sampler["enableAnisotropicFiltering"].as<bool>());
            }

            if (sampler["addressModeU"]) {
                auto it = m_samplerAddressModes.find(sampler["addressModeU"].as<std::string>());
                if (it == m_samplerAddressModes.end()) {
                    spdlog::error("ScinthDef {} has sampler VGen {} with unsupported addressModeU {}", name, className,
                                  sampler["addressModeU"].as<std::string>());
                    return nullptr;
                }
                samplerConfig.setAddressModeU(it->second);
            }

            if (sampler["addressModeV"]) {
                auto it = m_samplerAddressModes.find(sampler["addressModeV"].as<std::string>());
                if (it == m_samplerAddressModes.end()) {
                    spdlog::error("ScinthDef {} has sampler VGen {} with unsupported addressModeV {}", name, className,
                                  sampler["addressModeV"].as<std::string>());
                    return nullptr;
                }
                samplerConfig.setAddressModeV(it->second);
            }

            if (sampler["clampBorderColor"]) {
                auto it = m_samplerBorderColors.find(sampler["clampBorderColor"].as<std::string>());
                if (it == m_samplerBorderColors.end()) {
                    spdlog::error("ScinthDef {} has sampler VGen {} with unsupported clampBorderColor {}", name,
                                  className, sampler["clampBorderColor"].as<std::string>());
                    return nullptr;
                }
                samplerConfig.setClampBorderColor(it->second);
            }

            instance.setSamplerConfig(imageIndex, imageArgType, samplerConfig);
        } else {
            if (vgenClass->isSampler()) {
                spdlog::error("ScinthDef {} has sampler VGen {} with no sampler dictionary", name, className);
                return nullptr;
            }
        }

        if (vgen["tween"] && vgen["tween"].IsMap()) {
            if (!vgenClass->hasTween()) {
                spdlog::error("ScinthDef {} has non-tween VGen {} with tween dictionary", name, className);
                return nullptr;
            }

            auto tween = vgen["tween"];
            // Required key tween index.
            if (!tween["index"] || !tween["index"].IsScalar()) {
                spdlog::error("ScinthDef {} has sampler VGen {} missing image or imageArgType keys.", name, className);
                return nullptr;
            }
            instance.setTweenIndex(tween["index"].as<int>());
        }

        if (!vgen["outputs"] || !vgen["outputs"].IsSequence()) {
            spdlog::error("ScinthDef {} has vgen with className {} with absent or malformed outputs key", name,
                          className);
            return nullptr;
        }
        for (auto output : vgen["outputs"]) {
            if (!output.IsMap()) {
                spdlog::error("ScinthDef {} has VGen {} with non-map output.", name, className);
                return nullptr;
            }
            if (!output["dimension"] || !output["dimension"].IsScalar()) {
                spdlog::error("ScinthDef {} has VGen {} with absent or malformed dimension key.", name, className);
                return nullptr;
            }
            instance.addOutput(output["dimension"].as<int>());
        }

        if (vgen["inputs"]) {
            for (auto input : vgen["inputs"]) {
                if (!input.IsMap()) {
                    spdlog::error("ScinthDef {} has VGen {} with non-map input.", name, className);
                    return nullptr;
                }
                if (!input["type"] || !input["type"].IsScalar()) {
                    spdlog::error("ScinthDef {} has VGen {} input with absent or malformed type key.", name, className);
                    return nullptr;
                }

                if (!input["dimension"] || !input["dimension"].IsScalar()) {
                    spdlog::error("ScinthDef {} has VGen {} input with absent or malformed dimension key.", name,
                                  className);
                    return nullptr;
                }
                int dimension = input["dimension"].as<int>();

                std::string inputTypeString = input["type"].as<std::string>();
                auto typeIt = m_vgenInputTypes.find(inputTypeString);
                if (typeIt == m_vgenInputTypes.end()) {
                    spdlog::error("ScinthDef {} has VGen {} with undefined input type {}.", name, className,
                                  inputTypeString);
                    return nullptr;
                }
                VGen::InputType inputType = typeIt->second;
                switch (inputType) {
                case VGen::InputType::kConstant: {
                    if (!input["value"]) {
                        spdlog::error("ScinthDef {} has VGen {} constant input with no value key.", name, className);
                        return nullptr;
                    }
                    // TODO: higher-dimensional constants.
                    float constantValue = input["value"].as<float>();
                    instance.addConstantInput(constantValue);
                } break;

                case VGen::InputType::kVGen: {
                    if (!input["vgenIndex"] || !input["outputIndex"]) {
                        spdlog::error("ScinthDef {} has VGen {} vgen input with no vgenIndex or outputIndex key.", name,
                                      className);
                        return nullptr;
                    }
                    size_t vgenIndex = input["vgenIndex"].as<size_t>();
                    if (vgenIndex > instances.size()) {
                        spdlog::error("ScinthDef {} has VGen {} vgen input with invalid index {}.", name, className,
                                      vgenIndex);
                        return nullptr;
                    }
                    size_t outputIndex = input["outputIndex"].as<size_t>();
                    if (outputIndex >= instances[vgenIndex].abstractVGen()->outputs().size()) {
                        spdlog::error("ScinthDef {} has VGen {} vgen input with invalid output index {}.", name,
                                      className, outputIndex);
                        return nullptr;
                    }
                    instance.addVGenInput(vgenIndex, outputIndex, dimension);
                } break;

                case VGen::InputType::kParameter: {
                    if (!input["index"]) {
                        spdlog::error("ScinthDef {} has VGen {} parameter input with no index key.", name, className);
                        return nullptr;
                    }
                    size_t parameterIndex = input["index"].as<size_t>();
                    instance.addParameterInput(parameterIndex);
                } break;

                default: {
                    spdlog::error("ScinthDef {} has VGen {} parameter with unsupported input type.", name, className);
                    return nullptr;
                } break;
                }
            }
        }

        if (!instance.validate()) {
            spdlog::error("ScinthDef {} has invalid VGen {}.", name, className);
            return nullptr;
        }

        instances.push_back(instance);
    }

    std::shared_ptr<AbstractScinthDef> scinthDef(
        new AbstractScinthDef(name, std::move(shape), renderOptions, parameters, tweens, instances));
    return scinthDef;
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

        bool hasTween = false;
        if (node["tween"] && node["tween"].IsScalar()) {
            hasTween = node["tween"].as<bool>();
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
        std::vector<std::vector<size_t>> inputDimensions;
        std::vector<std::vector<size_t>> outputDimensions;
        for (auto dim : node["dimensions"]) {
            if (!dim.IsMap()) {
                spdlog::error("VGen {} has dimensions list element that is not a map.", name);
                break;
            }

            // The input tag is optional for those VGens that don't have inputs.
            std::vector<size_t> inputDims;
            if (dim["inputs"]) {
                // If only a single number provided use that as dimension for all inputs.
                if (dim["inputs"].IsScalar()) {
                    inputDims.insert(inputDims.begin(), inputs.size(), dim["inputs"].as<size_t>());
                } else if (dim["inputs"].IsSequence()) {
                    for (auto inDim : dim["inputs"]) {
                        inputDims.push_back(inDim.as<size_t>());
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
            std::vector<size_t> outputDims;
            if (dim["outputs"].IsScalar()) {
                outputDims.insert(outputDims.begin(), outputs.size(), dim["outputs"].as<size_t>());
            } else if (dim["outputs"].IsSequence()) {
                for (auto outDim : dim["outputs"]) {
                    outputDims.push_back(outDim.as<size_t>());
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
        for (size_t i = 0; i < outputDimensions.size(); ++i) {
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

        std::shared_ptr<AbstractVGen> vgen(new AbstractVGen(name, supportedRates, isSampler, hasTween, inputs, outputs,
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
