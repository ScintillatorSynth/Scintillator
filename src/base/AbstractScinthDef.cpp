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
    std::set<int> computeVGens;
    std::set<int> vertexVGens;
    std::set<int> fragmentVGens;

    std::random_device randomDevice;
    m_prefix = fmt::format("{}_{:08x}", m_name, randomDevice());

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
                m_instances[i].getInputParameterIndex(j, parameterIndex);
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
                    m_fragmentManifest.addElement(name, m_instances[vgenIndex].outputDimension(vgenOutput));
                    inputs.push_back(name);
                } break;

                case AbstractVGen::Rates::kFrame: {
                    std::string name = fmt::format("out_{}_{}", vgenIndex, vgenOutput);
                    m_drawUniformManifest.addElement(name, m_instances[vgenIndex].outputDimension(vgenOutput));
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

        // Build intrinsics.
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
                intrinsics[Intrinsic::kNormPos] = name;
            }   break;

            case kPi:
                intrinsics[Intrinsic::kPi] = "3.1415926535897932384626433832795f";
                break;

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

            case kTime:
                m_drawUniformManifest.addElement("time", Manifest::ElementType::kFloat, Intrinsic::kTime);
                instrinsics[Intrinsic::kTime] = fmt::format("{}_ubo.time", m_prefix);
                break;

            case kTexPos: {
                std::string name = fmt::format("{}_in_texPos", m_prefix);
                m_fragmentManifest.addElement(name, Manifest::ElementType::kVec2, Intrinsic::kTexPos);
                intrinsics[Intrinsic::kTexPos] = name;
            }    break;

            }
        }

        std::vector<std::string> outputs;
        for (auto j = 0; j < m_instances[index].abstractVGen()->outputs().size(); ++j) {
            outputs.emplace_back(fmt::format("{}_out_{}_{}", m_prefix, index, j);
        }

        m_fragmentShader += fmt::format("\n    // --- {}\n", m_instances[index].abstractVGen()->name());
        m_fragmentShader += fmt::format("    \n{}\n",
            m_instances[index].abstractVGen()->parameterize(inputs, intrinsics, outputs,
                                                              m_outputDimensions[index], alreadyDefined));
    }

    m_vertexShader = "";
    for (auto index : vertexVGens) {
        std::vector<std::string> inputs;
        for (auto j = 0; j < m_instances[index].numberOfInputs(); ++j) {
            VGen::InputType type = m_instances[index].getInputType(j);
            switch (type) {
            // same as fragment-rate
            case VGen::InputType::kConstant: {
                float constantValue;
                m_instances[index].getInputConstantValue(j, constantValue);
                inputs.emplace_back(fmt::format("{}", constantValue);
            } break;

            // same as fragment rate
            case VGen::InputType::kParameter: {
                int parameterIndex;
                m_instances[i].getInputParameterIndex(j, parameterIndex);
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
                    m_drawUniformManifest.addElement(name, m_instances[vgenIndex].outputDimension(vgenOutput));
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

        // Build intrinsics.
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
                instrinsics[Intrinsic::kTime] = fmt::format("{}_ubo.time", m_prefix);
                break;

            case kTexPos: {
                std::string name = fmt::format("{}_in_texPos", m_prefix);
                m_vertexManifest.addElement(name, Manifest::ElementType::kVec2, Intrinsic::kTexPos);
                intrinsics[Intrinsic::kTexPos] = name;
            }    break;

            }
        }

    }

    return true;
}

bool AbstractScinthDef::buildComputeStage(const std::set<int>& computeVGens) {
    // It's possible we have no frame-rate VGens, in which case we can elide the compute stage entirely.
    m_hasComputeStage = indices.size() > 0;
    if (!m_hasComputeStage) {
        return true;
    }

    std::string shaderMain = "void main() {\n";

    // Traverse set of compute VGens, adding to input and output manifest as needed.
    for (auto index : indices) {
        for (auto intrinsic : m_instances[index].abstractVGen()->intrinsics()) {
            m_computeIntrinsics.insert(intrinsic);

            switch (intrinsic) {
            case kNotFound:
                spdlog::error("ScinthDef {} has unknown intrinsic.", m_name);
                return false;

            case kNormPos:
                break;

            case kPi:
                break;

            case kSampler:
                break;

            case kTime:
                m_computeIntrinsics.addElement("time", Manifest::ElementType::kFloat, Intrinsic::kTime);
                break;

            case kTexPos:
                break;
            }
        }
    }

    return true;
}

bool AbstractScinthDef::finalizeShaders(const std::set<int>& computeVGens, const std::set<int>& vertexVGens,
        const std::set<int>& fragmentVGens) {
    // The fragment manifest should now be final, pack it.
    m_fragmentManifest.pack();

    m_fragmentShader = "#version 450\n"
                       "#extension GL_ARB_separate_shader_objects : enable\n"
                       "\n"
                       "// --- fragment shader inputs from vertex shader\n";

    // Now we add vertex shader outputs as described in the fragment Manifest, which will be both shape-rate VGen
    // outputs and pass-through intrinsics, as inputs to the fragment shader.

    // Uniform buffer description comes next, if one is present, containing frame-rate VGen outputs and intrinsics.

    // Fixed image samplers, followed by parameterized image samplers are declared here.

    // The push constant block for parameters is next.

    // Hard-coded single output which is color.
    m_fragmentShader += fmt::format("\nlayout(location = 0) out vec4 {};\n", m_fragmentOutputName);

    // Lastly the main fragment program.
    m_fragmentShader += "\n"
                        "void main() {"
                            + shaderMain +
                        "}\n";

    spdlog::info("{} fragment shader:\n{}", m_name, m_fragmentShader);
    return true;
}

std::string AbstractScinthDef::nameForVGenOutput(int vgenIndex, int outputIndex) const {
    if (vgenIndex < 0 || outputIndex >= m_instances.size()) {
        spdlog::error("Out of range request for VGenOutput in AbstractScinthDef");
        return std::string("");
    }
    if (vgenIndex == m_instances.size() - 1) {
        return m_fragmentOutputName;
    }
    return fmt::format("{}_out_{}_{}", m_prefix, vgenIndex, outputIndex);
}

int AbstractScinthDef::indexForParameterName(const std::string& name) const {
    auto it = m_parameterIndices.find(name);
    if (it == m_parameterIndices.end()) {
        return -1;
    }
    return it->second;
}

bool AbstractScinthDef::buildNames() {
    std::random_device randomDevice;
    m_prefix = fmt::format("{}_{:08x}", m_name, randomDevice());
    m_vertexPositionElementName = m_prefix + "_inPosition";
    m_fragmentOutputName = m_prefix + "_outColor";
    m_parametersStructName = m_prefix + "_parameters";
    for (auto i = 0; i < m_parameters.size(); ++i) {
        m_parameterIndices.insert(std::make_pair(m_parameters[i].name(), i));
    }

    // Build the parameters for all VGens.
    for (auto i = 0; i < m_instances.size(); ++i) {
        // First process inputs, plugging in either constants or outputs from other VGens as necessary.
        std::vector<std::string> vgenInputs;
        for (auto j = 0; j < m_instances[i].numberOfInputs(); ++j) {
            VGen::InputType type = m_instances[i].getInputType(j);
            switch (type) {
                // TODO: support for higher-dimensional constants? VGen has it, but we drop it here.
            case VGen::InputType::kConstant: {
                float constantValue;
                m_instances[i].getInputConstantValue(j, constantValue);
                vgenInputs.push_back(fmt::format("{}f", constantValue));
            } break;

            case VGen::InputType::kParameter: {
                int parameterIndex;
                m_instances[i].getInputParameterIndex(j, parameterIndex);
                vgenInputs.push_back(fmt::format("{}.{}", m_parametersStructName, m_parameters[parameterIndex].name()));
            } break;

            case VGen::InputType::kVGen: {
                int vgenIndex;
                int vgenOutput;
                // If a VGen index we use the output name of the VGen at that index.
                m_instances[i].getInputVGenIndex(j, vgenIndex, vgenOutput);
                vgenInputs.push_back(nameForVGenOutput(vgenIndex, vgenOutput));
            } break;

            case VGen::InputType::kInvalid:
                spdlog::error("AbstractScinthDesc {} VGen at index {} has unknown input type at index {}.", m_name, i,
                              j);
                return false;
            }
        }
        m_inputs.push_back(vgenInputs);

        // Collect all intrinsics for provision at runtime by the Scinth.
        for (auto intrinsic : m_instances[i].abstractVGen()->intrinsics()) {
            m_intrinsics.insert(intrinsic);
        }

        // Generate all output names.
        std::vector<std::string> vgenOutputs;
        for (auto j = 0; j < m_instances[i].abstractVGen()->outputs().size(); ++j) {
            vgenOutputs.push_back(nameForVGenOutput(i, j));
        }
        m_outputs.push_back(vgenOutputs);

        // Generate all output dimensions.
        std::vector<int> vgenOutputDimensions;
        for (auto j = 0; j < m_instances[i].numberOfOutputs(); ++j) {
            vgenOutputDimensions.push_back(m_instances[i].outputDimension(j));
        }
        m_outputDimensions.push_back(vgenOutputDimensions);
    }

    return true;
}

bool AbstractScinthDef::buildManifests() {
    // At minimum the vertex manifest must have the position data from the associated Shape.
    m_vertexManifest.addElement(m_vertexPositionElementName, m_shape->elementType());

    // Some intrinsics have manifest dependencies, process each in turn.
    for (Intrinsic intrinsic : m_intrinsics) {
        switch (intrinsic) {
        case kNotFound:
            spdlog::error("ScinthDef {} has unknown intrinsic.", m_name);
            return false;

        case kNormPos:
            // Double-check that this a 2D shape, normpos only works for 2D vertices.
            if (m_shape->elementType() != Manifest::ElementType::kVec2) {
                spdlog::error("normPos intrinsic only supported for 2D shapes in ScinthDef {}.", m_name);
                return false;
            }
            m_vertexManifest.addElement(m_prefix + "_normPos", Manifest::ElementType::kVec2, Intrinsic::kNormPos);
            break;

        case kPi:
            break;

        case kSampler:
            break;

        case kTime:
            m_uniformManifest.addElement("time", Manifest::ElementType::kFloat, Intrinsic::kTime);
            break;

        case kTexPos:
            m_vertexManifest.addElement(m_prefix + "_texPos", Manifest::ElementType::kVec2, Intrinsic::kTexPos);
            break;
        }
    }

    m_vertexManifest.pack();
    m_uniformManifest.pack();
    return true;
}

bool AbstractScinthDef::buildVertexShader() {
    // Start with standardized shader header.
    m_vertexShader = "#version 450\n"
                     "#extension GL_ARB_separate_shader_objects : enable\n"
                     "\n"
                     "// --- vertex shader inputs\n";

    // Describe all inputs to the vertex shader via vertex data.
    for (auto i = 0; i < m_vertexManifest.numberOfElements(); ++i) {
        // TODO: hard-coded assumption that all inputs take 1 slot
        m_vertexShader += fmt::format("layout(location = {}) in {} in_{};\n", i, m_vertexManifest.typeNameForElement(i),
                                      m_vertexManifest.nameForElement(i));
    }

    m_vertexShader += "\n"
                      "// --- vertex shader outputs\n";

    // Now produce the vertex shader outputs. TODO: allowing vertex shader VGens will require more data here. But for
    // now we just copy everything to the fragment shader except for the _inPosition, which gets assigned to the
    // keyword gl_Position.
    for (auto i = 0; i < m_vertexManifest.numberOfElements(); ++i) {
        if (m_vertexManifest.nameForElement(i) != m_vertexPositionElementName) {
            m_vertexShader += fmt::format("layout(location = {}) out {} out_{};\n", i,
                                          m_vertexManifest.typeNameForElement(i), m_vertexManifest.nameForElement(i));
        }
    }

    // TODO: uniform is fragment-only for now.

    m_vertexShader += "\n"
                      "void main() {\n";

    // Assign all input elements to their respective output elements.
    for (auto i = 0; i < m_vertexManifest.numberOfElements(); ++i) {
        if (m_vertexManifest.nameForElement(i) == m_vertexPositionElementName) {
            switch (m_vertexManifest.typeForElement(i)) {
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
        } else {
            m_vertexShader += fmt::format("    out_{} = in_{};\n", m_vertexManifest.nameForElement(i),
                                          m_vertexManifest.nameForElement(i));
        }
    }

    m_vertexShader += "}\n";
    spdlog::info("{} vertex shader:\n{}", m_name, m_vertexShader);
    return true;
}

bool AbstractScinthDef::buildFragmentShader() {
    // Start with standardized header.
    m_fragmentShader = "#version 450\n"
                       "#extension GL_ARB_separate_shader_objects : enable\n"
                       "\n"
                       "// --- fragment shader inputs from vertex shader\n";

    // For now, all intrinsics are global, coming from either the vertex shader or the uniform buffer, so we can define
    // a single map with all of their substitutions.
    std::unordered_map<Intrinsic, std::string> intrinsicNames;
    intrinsicNames.insert({ Intrinsic::kFragCoord, "gl_FragCoord" });
    intrinsicNames.insert({ Intrinsic::kPi, "3.1415926535897932384626433832795f" });

    // Now add any inputs that might have come from the vertex shader by processing the vertex manifest.
    for (auto i = 0; i < m_vertexManifest.numberOfElements(); ++i) {
        if (m_vertexManifest.nameForElement(i) != m_vertexPositionElementName) {
            m_fragmentShader += fmt::format("layout(location = {}) in {} in_{};\n", i,
                                            m_vertexManifest.typeNameForElement(i), m_vertexManifest.nameForElement(i));
            Intrinsic intrinsic = m_vertexManifest.intrinsicForElement(i);
            switch (intrinsic) {
            case kPi:
            case kSampler:
                break;

            case kNormPos:
            case kTexPos:
            case kTime:
                intrinsicNames.insert({ intrinsic, "in_" + m_vertexManifest.nameForElement(i) });
                break;

            case kNotFound:
                spdlog::error("Unknown fragment shader vertex input {}", m_vertexManifest.nameForElement(i));
                return false;
            }
        }
    }

    // If there's a uniform buffer build that next.
    int binding = 0;
    if (m_uniformManifest.numberOfElements()) {
        m_fragmentShader += fmt::format("\n"
                                        "// --- fragment shader uniform buffer\n"
                                        "layout(binding = {}) uniform UBO {{\n",
                                        binding);
        ++binding;
        for (auto i = 0; i < m_uniformManifest.numberOfElements(); ++i) {
            switch (m_uniformManifest.intrinsicForElement(i)) {
            case kNormPos:
                spdlog::error("normPos not supported as uniform buffer argument in ScinthDef {}", m_name);
                return false;

            case kNotFound:
                spdlog::error("undefined intrinsic in uniform buffer in ScinthDef {}", m_name);
                return false;

            default:
                m_fragmentShader += fmt::format("    {} {};\n", m_uniformManifest.typeNameForElement(i),
                                                m_uniformManifest.nameForElement(i));
                intrinsicNames.insert({ m_uniformManifest.intrinsicForElement(i),
                                        fmt::format("{}_ubo.{}", m_prefix, m_uniformManifest.nameForElement(i)) });
                break;
            }
        }

        m_fragmentShader += fmt::format("}} {}_ubo;\n", m_prefix);
    }

    // Constant and parameterized Sampler inputs come next, using the combined sampler/image binding which may be faster
    // on some Vulkan implementations.
    if (m_fixedImages.size()) {
        m_fragmentShader += "\n"
                            "// --- fixed image sampler inputs\n";
        for (auto pair : m_fixedImages) {
            m_fragmentShader += fmt::format("layout(binding = {}) uniform sampler2D {}_sampler_{:08x}_fixed_{};\n",
                                            binding, m_prefix, pair.first, pair.second);
            ++binding;
        }
    }

    if (m_parameterizedImages.size()) {
        m_fragmentShader += "\n"
                            "// --- parammeterized image sampler inputs\n";
        for (auto pair : m_parameterizedImages) {
            m_fragmentShader += fmt::format("layout(binding = {}) uniform sampler2D {}_sampler_{:08x}_param_{};\n",
                                            binding, m_prefix, pair.first, pair.second);
            ++binding;
        }
    }

    // We pack the parameters into a push constant structure.
    if (m_parameters.size()) {
        m_fragmentShader += "\n"
                            "// --- fragment shader parameter push constants\n"
                            "layout(push_constant) uniform parametersBlock {\n";

        for (const auto& param : m_parameters) {
            m_fragmentShader += fmt::format("  float {};\n", param.name());
        }

        m_fragmentShader += fmt::format("}} {};\n", m_parametersStructName);
    }

    // Hard-coded single output which is color.
    m_fragmentShader += fmt::format("\nlayout(location = 0) out vec4 {};\n", m_fragmentOutputName);

    m_fragmentShader += "\n"
                        "void main() {";

    std::unordered_set<std::string> alreadyDefined({ m_fragmentOutputName });
    for (auto i = 0; i < m_instances.size(); ++i) {
        // Update instance-specific intrinsics.
        // TODO: consider providing a separate map or an alternate way of providing global and local intrinsic names.
        if (m_instances[i].abstractVGen()->isSampler()) {
            if (m_instances[i].imageArgType() == VGen::InputType::kConstant) {
                intrinsicNames[Intrinsic::kSampler] =
                    fmt::format("{}_sampler_{:08x}_fixed_{}", m_prefix, m_instances[i].sampler().key(),
                                m_instances[i].imageIndex());
            } else {
                intrinsicNames[Intrinsic::kSampler] =
                    fmt::format("{}_sampler_{:08x}_param_{}", m_prefix, m_instances[i].sampler().key(),
                                m_instances[i].imageIndex());
            }
        }
        m_fragmentShader += "\n    // --- " + m_instances[i].abstractVGen()->name() + "\n";
        m_fragmentShader += "    "
            + m_instances[i].abstractVGen()->parameterize(m_inputs[i], intrinsicNames, m_outputs[i],
                                                          m_outputDimensions[i], alreadyDefined)
            + "\n";
    }

    m_fragmentShader += "}\n";

    spdlog::info("{} fragment shader:\n{}", m_name, m_fragmentShader);
    return true;
}

} // namespace base

} // namespace scin
