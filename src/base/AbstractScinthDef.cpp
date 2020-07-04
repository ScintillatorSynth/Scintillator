#include "base/AbstractScinthDef.hpp"

#include "base/Shape.hpp"
#include "base/VGen.hpp"

#include "fmt/core.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include <random>

/*!
 * Information flow within a ScinthDef
 * -----------------------------------
 *
 * Compute shaders, if there are any, are scheduled separately and issued in their own consolidated command buffer.
 * Their output needs to go to a uniform buffer, with one maintained for each pipelined frame. This can be the same
 * uniform that holds intrinsics like @time, it can also hold outputs from the compute uniform. The uniform will need
 * to be adjusted for vertex and fragment access.
 *
 * Vertex computations are per-vertex, and output will always flow to the fragment shader. So inputs to vertex shader
 * come from uniform, outputs must be reserved per vertex instance, so they can be provided to the fragment.
 *
 * Outputs from AbstractScinthDef processing are compute, vertex, fragment shaders. Manifests for compute input via
 * uniform buffer and manifest for vertex/fragment uniform buffer. Manifest for vertex input, and a manifest for vertex
 * output which is also part of fragment input.
 *
 * There also unified samplers and push constants.
 *
 */

namespace scin { namespace base {

AbstractScinthDef::AbstractScinthDef(const std::string& name, const std::vector<Parameter>& parameters,
                                     const std::vector<VGen>& instances):
    m_name(name),
    m_parameters(parameters),
    m_instances(instances),
    m_shape(new Quad()),
    m_hasComputeStage(false) {}

AbstractScinthDef::~AbstractScinthDef() { spdlog::debug("AbstractScinthDef '{}' destructor", m_name); }

bool AbstractScinthDef::build() {
    std::random_device randomDevice;
    m_prefix = fmt::format("{}_{:08x}", m_name, randomDevice());
    m_vertexPositionElementName = m_prefix + "_inPosition";
    m_fragmentOutputName = m_prefix + "_outColor";

    std::set<int> computeVGens;
    std::set<int> vertexVGens;
    std::set<int> fragmentVGens;
    if (!groupVGens(m_instances.size() - 1, AbstractVGen::Rates::kPixel, computeVGens, vertexVGens, fragmentVGens)) {
        return false;
    }
    if (!buildDrawStage(vertexVGens, fragmentVGens)) {
        return false;
    }
    if (!buildComputeStage(computeVGens)) {
        return false;
    }
    if (!finalizeShaders(computeVGens, vertexVGens, fragmentVGens)) {
        return false;
    }

    return true;
}

bool AbstractScinthDef::groupVGens(int index, AbstractVGen::Rates maxRate, std::set<int>& computeVGens,
                                   std::set<int>& vertexVGens, std::set<int>& fragmentVGens) {
    AbstractVGen::Rates vgenRate = m_instances[index].rate();

    // Bucket VGen index and validate rate is supported value.
    switch (vgenRate) {
    case AbstractVGen::Rates::kFrame:
        computeVGens.insert(index);
        break;

    case AbstractVGen::Rates::kShape:
        vertexVGens.insert(index);
        break;

    case AbstractVGen::Rates::kPixel:
        fragmentVGens.insert(index);
        break;

    default:
        spdlog::error("Invalid or absent VGen rate on ScinthDef {} at index {}.", m_name, index);
        return false;
    }

    // Validate rate of this VGen against the rate of the downstream VGen, to ensure rate progression.
    if (vgenRate > maxRate) {
        spdlog::error("Invalid rate change on ScinthDef {} at VGen index {}.", m_name, index);
        return false;
    }

    // Extract image parameters if this is a sampling VGen.
    if (m_instances[index].abstractVGen()->isSampler()) {
        if (m_instances[index].imageIndex() < 0) {
            spdlog::error("AbstractScinthDef {} has VGen {} with bad image index.", m_name, index);
            return false;
        }
        if (m_instances[index].imageArgType() == VGen::InputType::kConstant) {
            if (vgenRate == AbstractVGen::Rates::kFrame) {
                m_computeFixedImages.insert({ m_instances[index].sampler().key(), m_instances[index].imageIndex() });
            } else {
                m_drawFixedImages.insert({ m_instances[index].sampler().key(), m_instances[index].imageIndex() });
            }
        } else if (m_instances[index].imageArgType() == VGen::InputType::kParameter) {
            if (vgenRate == AbstractVGen::Rates::kFrame) {
                m_computeParameterizedImages.insert(
                    { m_instances[index].sampler().key(), m_instances[index].imageIndex() });
            } else {
                m_drawParameterizedImages.insert(
                    { m_instances[index].sampler().key(), m_instances[index].imageIndex() });
            }
        } else {
            spdlog::error("AbstractScinthDef {} has unknown VGen {} with sampler image argument type.", m_name, index);
            return false;
        }
    }

    // Recurse up the graph, propagating any errors encountered.
    for (auto i = 0; i < m_instances[index].numberOfInputs(); ++i) {
        if (m_instances[index].getInputType(i) == VGen::InputType::kVGen) {
            int vgenIndex, vgenOutput;
            m_instances[index].getInputVGenIndex(i, vgenIndex, vgenOutput);
            if (!groupVGens(vgenIndex, vgenRate, computeVGens, vertexVGens, fragmentVGens)) {
                return false;
            }
        }
    }

    return true;
}

bool AbstractScinthDef::buildDrawStage(const std::set<int>& vertexVGens, const std::set<int>& fragmentVGens) {
    m_fragmentShader = "";
    for (auto index : fragmentVGens) {
        // Build input names (for parameterization) and add to relevant manifests as needed.
        std::vector<std::string> inputs;
        for (auto j = 0; j < m_instances[index].numberOfInputs(); ++j) {
            VGen::InputType type = m_instances[index].getInputType(j);
            switch (type) {
                // TODO: support for higher-dimensional constants? VGen has it, but we drop it here.
                // TODO: constants and parameters are going to be common across fragment, vertex, and compute shaders.
            case VGen::InputType::kConstant: {
                float constantValue;
                m_instances[index].getInputConstantValue(j, constantValue);
                inputs.emplace_back(fmt::format("{}f", constantValue));
            } break;

            case VGen::InputType::kParameter: {
                int parameterIndex;
                m_instances[index].getInputParameterIndex(j, parameterIndex);
                inputs.emplace_back(fmt::format("{}_parameters.{}", m_prefix, m_parameters[parameterIndex].name()));
            } break;

            case VGen::InputType::kVGen: {
                int vgenIndex, vgenOutput;
                m_instances[index].getInputVGenIndex(j, vgenIndex, vgenOutput);
                switch (m_instances[vgenIndex].rate()) {
                case AbstractVGen::Rates::kPixel:
                    inputs.emplace_back(fmt::format("{}_out_{}_{}", m_prefix, vgenIndex, vgenOutput));
                    break;

                case AbstractVGen::Rates::kShape: {
                    std::string name = fmt::format("{}_out_{}_{}", m_prefix, vgenIndex, vgenOutput);
                    m_fragmentManifest.addElement(name, static_cast<Manifest::ElementType>(
                                m_instances[vgenIndex].outputDimension(vgenOutput)));
                    inputs.push_back(name);
                } break;

                case AbstractVGen::Rates::kFrame: {
                    std::string name = fmt::format("out_{}_{}", vgenIndex, vgenOutput);
                    m_drawUniformManifest.addElement(name, static_cast<Manifest::ElementType>(
                                m_instances[vgenIndex].outputDimension(vgenOutput)));
                    inputs.emplace_back(fmt::format("{}_ubo.{}", m_prefix, name));
                } break;

                default:
                    spdlog::error("Unsupported rate encountered in fragment stage of ScinthDef {}", m_name);
                    return false;
                }
            } break;

            case VGen::InputType::kInvalid:
                spdlog::error("AbstractScinthDesc {} VGen at index {} has unknown input type at index {}.", m_name,
                              index, j);
                return false;
            }
        }

        // Build fragment intrinsics.
        std::unordered_map<Intrinsic, std::string> intrinsics;
        for (auto intrinsic : m_instances[index].abstractVGen()->intrinsics()) {
            switch (intrinsic) {
            case kFragCoord:
                intrinsics[Intrinsic::kFragCoord] = "gl_FragCoord";
                break;

            case kNotFound:
                spdlog::error("ScinthDef {} has unknown intrinsic.", m_name);
                return false;

            case kNormPos: {
                std::string name = fmt::format("{}_in_normPos", m_prefix);
                m_fragmentManifest.addElement(name, Manifest::ElementType::kVec2, Intrinsic::kNormPos);
                m_vertexManifest.addElement(name, Manifest::ElementType::kVec2, Intrinsic::kNormPos);
                intrinsics[Intrinsic::kNormPos] = name;
            }   break;

            case kPi:
                intrinsics[Intrinsic::kPi] = "3.1415926535897932384626433832795f";
                break;

            case kSampler: {
                if (m_instances[index].imageArgType() == VGen::InputType::kConstant) {
                    intrinsics[Intrinsic::kSampler] =
                        fmt::format("{}_sampler_{:08x}_fixed_{}", m_prefix, m_instances[index].sampler().key(),
                                    m_instances[index].imageIndex());
                } else {
                    intrinsics[Intrinsic::kSampler] =
                        fmt::format("{}_sampler_{:08x}_param_{}", m_prefix, m_instances[index].sampler().key(),
                                    m_instances[index].imageIndex());
                }
            }   break;

            case kTime:
                m_drawUniformManifest.addElement("time", Manifest::ElementType::kFloat, Intrinsic::kTime);
                intrinsics[Intrinsic::kTime] = fmt::format("{}_ubo.time", m_prefix);
                break;

            case kTexPos: {
                std::string name = fmt::format("{}_in_texPos", m_prefix);
                m_fragmentManifest.addElement(name, Manifest::ElementType::kVec2, Intrinsic::kTexPos);
                m_vertexManifest.addElement(name, Manifest::ElementType::kVec2, Intrinsic::kTexPos);
                intrinsics[Intrinsic::kTexPos] = name;
            }    break;

            }
        }

        std::vector<std::string> outputs;
        std::vector<int> outputDimensions;
        for (auto j = 0; j < m_instances[index].numberOfOutputs(); ++j) {
            if (index < m_instances.size() - 1 || j > 0) {
                outputs.emplace_back(fmt::format("{}_out_{}_{}", m_prefix, index, j));
            } else {
                outputs.emplace_back(m_fragmentOutputName);
            }
            outputDimensions.push_back(m_instances[index].outputDimension(j));
        }

        std::unordered_set<std::string> alreadyDefined({ m_fragmentOutputName });

        m_fragmentShader += fmt::format("\n    // --- {}\n", m_instances[index].abstractVGen()->name());
        m_fragmentShader += fmt::format("    {}\n",
            m_instances[index].abstractVGen()->parameterize(inputs, intrinsics, outputs,
                                                            outputDimensions, alreadyDefined));
    }

    m_vertexShader = "";

    // Add position element to vertex manifest.
    m_vertexManifest.addElement(m_vertexPositionElementName, m_shape->elementType());

    for (auto index : vertexVGens) {
        std::vector<std::string> inputs;
        for (auto j = 0; j < m_instances[index].numberOfInputs(); ++j) {
            VGen::InputType type = m_instances[index].getInputType(j);
            switch (type) {
            // same as fragment-rate
            case VGen::InputType::kConstant: {
                float constantValue;
                m_instances[index].getInputConstantValue(j, constantValue);
                inputs.emplace_back(fmt::format("{}", constantValue));
            } break;

            // same as fragment rate
            case VGen::InputType::kParameter: {
                int parameterIndex;
                m_instances[index].getInputParameterIndex(j, parameterIndex);
                inputs.emplace_back(fmt::format("{}_parameters.{}", m_prefix, m_parameters[parameterIndex].name()));
            } break;

            case VGen::InputType::kVGen: {
                int vgenIndex, vgenOutput;
                m_instances[index].getInputVGenIndex(j, vgenIndex, vgenOutput);
                switch (m_instances[vgenIndex].rate()) {
                case AbstractVGen::Rates::kPixel:
                    spdlog::error("Shape-rate VGen index {} with fragment-rate input index {}", index, vgenIndex);
                    return false;

                case AbstractVGen::Rates::kShape: {
                    inputs.push_back(fmt::format("{}_out_{}_{}", m_prefix, vgenIndex, vgenOutput));
                } break;

                // same as fragment rate
                case AbstractVGen::Rates::kFrame: {
                    std::string name = fmt::format("out_{}_{}", vgenIndex, vgenOutput);
                    m_drawUniformManifest.addElement(name, static_cast<Manifest::ElementType>(
                                m_instances[vgenIndex].outputDimension(vgenOutput)));
                    inputs.emplace_back(fmt::format("{}_ubo.{}", m_prefix, name));
                } break;

                default:
                    spdlog::error("Unsupported rate encountered in vertex stage of ScinthDef {}", m_name);
                    return false;
                }
            } break;

            case VGen::InputType::kInvalid:
                spdlog::error("AbstractScinthDesc {} VGen at index {} has unknown input type at index {}.", m_name,
                              index, j);
                return false;
            }
        }

        // Build vertex intrinsics.
        std::unordered_map<Intrinsic, std::string> intrinsics;
        for (auto intrinsic : m_instances[index].abstractVGen()->intrinsics()) {
            switch (intrinsic) {
            case kFragCoord:
                spdlog::error("@fragCoord intrinsic not supported in shape-rate VGen index {}", index);
                break;

            case kNotFound:
                spdlog::error("ScinthDef {} has unknown intrinsic.", m_name);
                return false;

            case kNormPos: {
                std::string name = fmt::format("{}_in_normPos", m_prefix);
                m_vertexManifest.addElement(name, Manifest::ElementType::kVec2, Intrinsic::kNormPos);
                intrinsics[Intrinsic::kNormPos] = name;
            }   break;

            // same as fragment rate
            case kPi:
                intrinsics[Intrinsic::kPi] = "3.1415926535897932384626433832795f";
                break;

            // same as fragment rate
            case kSampler:
                if (m_instances[index].imageArgType() == VGen::InputType::kConstant) {
                    intrinsics[Intrinsic::kSampler] =
                        fmt::format("{}_sampler_{:08x}_fixed_{}", m_prefix, m_instances[index].sampler().key(),
                                    m_instances[index].imageIndex());
                } else {
                    intrinsics[Intrinsic::kSampler] =
                        fmt::format("{}_sampler_{:08x}_param_{}", m_prefix, m_instances[index].sampler().key(),
                                    m_instances[index].imageIndex());
                }
                break;

            // same as fragment rate
            case kTime:
                m_drawUniformManifest.addElement("time", Manifest::ElementType::kFloat, Intrinsic::kTime);
                intrinsics[Intrinsic::kTime] = fmt::format("{}_ubo.time", m_prefix);
                break;

            case kTexPos: {
                std::string name = fmt::format("{}_in_texPos", m_prefix);
                m_vertexManifest.addElement(name, Manifest::ElementType::kVec2, Intrinsic::kTexPos);
                intrinsics[Intrinsic::kTexPos] = name;
            }    break;

            }
        }

        std::vector<std::string> outputs;
        std::vector<int> outputDimensions;
        for (auto j = 0; j < m_instances[index].numberOfOutputs(); ++j) {
            outputs.emplace_back(fmt::format("{}_out_{}_{}", m_prefix, index, j));
            outputDimensions.push_back(m_instances[index].outputDimension(j));
        }

        std::unordered_set<std::string> alreadyDefined;

        m_vertexShader += fmt::format("\n    // --- {}\n", m_instances[index].abstractVGen()->name());
        m_vertexShader += fmt::format("    {}\n",
            m_instances[index].abstractVGen()->parameterize(inputs, intrinsics, outputs,
                                                            outputDimensions, alreadyDefined));
    }

    return true;
}

bool AbstractScinthDef::buildComputeStage(const std::set<int>& computeVGens) {
    // It's possible we have no frame-rate VGens, in which case we can elide the compute stage entirely.
    m_hasComputeStage = computeVGens.size() > 0;
    if (!m_hasComputeStage) {
        return true;
    }

    return true;
}

bool AbstractScinthDef::finalizeShaders(const std::set<int>& computeVGens, const std::set<int>& vertexVGens,
        const std::set<int>& fragmentVGens) {
    // Pack all the manifests as we have analyzed all VGens so they should all be final now.
    m_fragmentManifest.pack();
    m_drawUniformManifest.pack();
    m_vertexManifest.pack();
    m_computeUniformManifest.pack();

    // Fragment and Vertex headers share many similar inputs so we build them together.
    std::string vertexHeader = "#version 450\n"
                               "#extension GL_ARB_separate_shader_objects : enable\n";
    std::string fragmentHeader = vertexHeader;

    vertexHeader += "\n// --- vertex shader inputs from vertex format\n";
    // Describe all inputs to the vertex shader via vertex data.
    for (auto i = 0; i < m_vertexManifest.numberOfElements(); ++i) {
        // note that hard-coded assumption that all inputs take 1 slot likely won't work for matrices
        vertexHeader += fmt::format("layout(location = {}) in {} in_{};\n", i, m_vertexManifest.typeNameForElement(i),
                                    m_vertexManifest.nameForElement(i));
    }

    if (m_fragmentManifest.numberOfElements()) {
        // Now we add vertex shader outputs as described in the fragment Manifest, which will be both shape-rate VGen
        // outputs and pass-through intrinsics, as inputs to the fragment shader.
        vertexHeader += "\n// -- vertex shader outputs to fragment shader\n";
        fragmentHeader += "\n// --- fragment shader inputs from vertex shader\n";
        for (auto i = 0; i < m_fragmentManifest.numberOfElements(); ++i) {
            if (m_fragmentManifest.nameForElement(i) != m_vertexPositionElementName) {
                fragmentHeader += fmt::format("layout(location = {}) in {} in_{};\n", i,
                        m_fragmentManifest.typeNameForElement(i), m_fragmentManifest.nameForElement(i));
                vertexHeader += fmt::format("layout(location = {}) out {} out_{};\n", i,
                        m_fragmentManifest.typeNameForElement(i), m_fragmentManifest.nameForElement(i));
                // We add the copy commands to the vertex shader now, if these are intrinsics that need to be copied.
                switch (m_fragmentManifest.intrinsicForElement(i)) {
                case Intrinsic::kNormPos:
                    m_vertexShader += fmt::format("    out_{}
                    break;
                case Intrinsic::kTexPos:
                    break;
                default:
                    break;
                }
            }
        }
    }

    // Uniform buffer description comes next, if one is present, containing frame-rate VGen outputs and intrinsics.
    int binding = 0;
    if (m_drawUniformManifest.numberOfElements()) {
        std::string uboBody;
        for (auto i = 0; i < m_drawUniformManifest.numberOfElements(); ++i) {
            uboBody += fmt::format("    {} {};\n", m_drawUniformManifest.typeNameForElement(i),
                                   m_drawUniformManifest.nameForElement(i));
        }
        vertexHeader += fmt::format("\n"
                                    "// -- vertex shader uniform buffer\n"
                                    "layout(binding = {}) uniform UBO {{\n",
                                    "{}"
                                    "}} {}_ubo;\n",
                                    binding, uboBody, m_prefix);
        fragmentHeader += fmt::format("\n"
                                      "// --- fragment shader uniform buffer\n"
                                      "layout(binding = {}) uniform UBO {{\n",
                                      "{}"
                                      "}} {}_ubo;\n",
                                      binding, uboBody, m_prefix);
        ++binding;
    }

    // Fixed image samplers, followed by parameterized image samplers are declared here.
    if (m_drawFixedImages.size()) {
        std::string samplerBody;
        for (auto pair : m_drawFixedImages) {
            samplerBody += fmt::format("layout(binding = {}) uniform sampler2D {}_sampler_{:08x}_fixed_{};\n",
                                          binding, m_prefix, pair.first, pair.second);
            ++binding;
        }
        vertexHeader += "\n"
                        "// -- fixed image sampler inputs\n" + samplerBody;
        fragmentHeader += "\n"
                          "// --- fixed image sampler inputs\n" + samplerBody;
    }

    if (m_drawParameterizedImages.size()) {
        std::string samplerBody;
        for (auto pair : m_drawParameterizedImages) {
            samplerBody += fmt::format("layout(binding = {}) uniform sampler2D {}_sampler_{:08x}_param_{};\n",
                                        binding, m_prefix, pair.first, pair.second);
            ++binding;
        }
        vertexHeader += "\n"
                        "// --- parammeterized image sampler inputs\n" + samplerBody;
        fragmentHeader += "\n"
                           "// --- parammeterized image sampler inputs\n" + samplerBody;
    }

    // We pack the parameters into a push constant structure.
    if (m_parameters.size()) {
        std::string paramBody;
        for (const auto& param : m_parameters) {
            paramBody += fmt::format("  float {};\n", param.name());
        }
        vertexHeader += fmt::format("\n"
                                    "// --- fragment shader parameter push constants\n"
                                    "layout(push_constant) uniform parametersBlock {\n"
                                    "{}"
                                    "}} {};\n", paramBody, m_parametersStructName);

        fragmentHeader += fmt::format("\n"
                                      "// --- fragment shader parameter push constants\n"
                                      "layout(push_constant) uniform parametersBlock {\n"
                                      "{}"
                                      "}} {};\n", paramBody, m_parametersStructName);
    }

    // Finally set the gl_Position vertex output based on input type.
    m_vertexShader += "\n    // --- hard-coded vertex position output.\n";
    switch (m_shape->elementType()) {
    case Manifest::ElementType::kFloat:
        m_vertexShader +=
            fmt::format("    gl_Position = vec4(in_{}, 0.0f, 0.0f, 1.0f);\n", m_vertexPositionElementName);
        break;

    case Manifest::ElementType::kVec2:
        m_vertexShader +=
            fmt::format("    gl_Position = vec4(in_{}, 0.0f, 1.0f);\n", m_vertexPositionElementName);
        break;

    case Manifest::ElementType::kVec3:
        m_vertexShader += fmt::format("    gl_Position = vec4(in_{}, 1.0f);\n", m_vertexPositionElementName);
        break;

    case Manifest::ElementType::kVec4:
        m_vertexShader += fmt::format("    gl_Position = in_{};\n", m_vertexPositionElementName);
        break;
    }

    m_vertexShader = vertexHeader +
                     "\n"
                     "void main() {"
                        + m_vertexShader +
                    "}\n";

    // Hard-coded single fragment output which is color.
    fragmentHeader += "\n// --- fragment output color\n";
    fragmentHeader += fmt::format("layout(location = 0) out vec4 {};\n", m_fragmentOutputName);

    m_fragmentShader = fragmentHeader +
                        "\n"
                        "void main() {"
                            + m_fragmentShader +
                        "}\n";

    spdlog::info("{} vertex shader:\n{}", m_name, m_vertexShader);
    spdlog::info("{} fragment shader:\n{}", m_name, m_fragmentShader);
    return true;
}

int AbstractScinthDef::indexForParameterName(const std::string& name) const {
    auto it = m_parameterIndices.find(name);
    if (it == m_parameterIndices.end()) {
        return -1;
    }
    return it->second;
}

} // namespace base

} // namespace scin
