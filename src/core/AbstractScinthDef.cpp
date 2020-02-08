#include "core/AbstractScinthDef.hpp"

#include "core/AbstractVGen.hpp"
#include "core/Shape.hpp"
#include "core/VGen.hpp"

#include "fmt/core.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include <random>

namespace scin { namespace core {

AbstractScinthDef::AbstractScinthDef(const std::string& name, const std::vector<Parameter>& parameters,
                                     const std::vector<VGen>& instances):
    m_name(name),
    m_parameters(parameters),
    m_instances(instances),
    m_shape(new Quad()) {}

AbstractScinthDef::~AbstractScinthDef() { spdlog::debug("AbstractScinthDef '{}' destructor", m_name); }

bool AbstractScinthDef::build() {
    if (!buildNames()) {
        return false;
    }
    if (!buildManifests()) {
        return false;
    }
    if (!buildVertexShader()) {
        return false;
    }
    if (!buildFragmentShader()) {
        return false;
    }
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

            case VGen::InputType::kInvalid: {
                spdlog::error("AbstractScinthDesc {} VGen at index {} has unknown input type at index {}.", m_name, i,
                              j);
            }
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

    // Other Intrinsics have manifest dependencies, process each in turn.
    for (Intrinsic intrinsic : m_intrinsics) {
        switch (intrinsic) {
        case kNormPos:
            // Double-check that this a 2D shape, normpos only works for 2D vertices.
            if (m_shape->elementType() != Manifest::ElementType::kVec2) {
                spdlog::error("normpos intrinsic only supported for 2D shapes in ScinthDef {}.", m_name);
                return false;
            }
            m_vertexManifest.addElement(m_prefix + "_normPos", Manifest::ElementType::kVec2, Intrinsic::kNormPos);
            break;

        case kPi:
            break;

        case kTime:
            m_uniformManifest.addElement("time", Manifest::ElementType::kFloat, Intrinsic::kTime);
            break;

        default:
            spdlog::error("invalid intrinsic while building manifest");
            return false;
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
    intrinsicNames.insert({ Intrinsic::kPi, "3.1415926535897932384626433832795f" });

    // Now add any inputs that might have come from the vertex shader by processing the vertex manifest.
    for (auto i = 0; i < m_vertexManifest.numberOfElements(); ++i) {
        if (m_vertexManifest.nameForElement(i) != m_vertexPositionElementName) {
            m_fragmentShader += fmt::format("layout(location = {}) in {} in_{};\n", i,
                                            m_vertexManifest.typeNameForElement(i), m_vertexManifest.nameForElement(i));
            Intrinsic intrinsic = m_vertexManifest.intrinsicForElement(i);
            switch (intrinsic) {
            case kPi:
                break;

            case kNormPos:
            case kTime:
                intrinsicNames.insert({ intrinsic, "in_" + m_vertexManifest.nameForElement(i) });
                break;

            case kNotFound:
                spdlog::warn("unknown fragment shader vertex input {}", m_vertexManifest.nameForElement(i));
                break;
            }
            if (intrinsic == Intrinsic::kNotFound) {
            } else {
                intrinsicNames.insert({ intrinsic, "in_" + m_vertexManifest.nameForElement(i) });
            }
        }
    }

    // If there's a uniform buffer build that next.
    if (m_uniformManifest.numberOfElements()) {
        m_fragmentShader += "\n"
                            "// --- fragment shader uniform buffer\n"
                            "layout(binding = 0) uniform UBO {\n";
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

} // namespace core

} // namespace scin
