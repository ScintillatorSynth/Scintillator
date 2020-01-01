#include "core/AbstractScinthDef.hpp"

#include "core/AbstractVGen.hpp"
#include "core/Shape.hpp"
#include "core/VGen.hpp"

#include "fmt/core.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include <random>

namespace scin {

AbstractScinthDef::AbstractScinthDef(const std::string& name, const std::vector<VGen>& instances):
    m_name(name),
    m_instances(instances),
    m_shape(new Quad()) {}

AbstractScinthDef::~AbstractScinthDef() {}

bool AbstractScinthDef::build() {
    std::random_device randomDevice;
    m_prefix = fmt::format("{}_{:8x}", m_name, randomDevice());

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
        return std::string("");
    }
    // TODO: fix hard-coded assumption here about gl_FragColor
    if (vgenIndex == m_instances.size() - 1) {
        return std::string("gl_FragColor");
    }
    return fmt::format("{}_out_{}_{}", m_prefix, vgenIndex, outputIndex);
}

bool AbstractScinthDef::buildNames() {
    // Build the parameters for all VGens.
    for (auto i = 0; i < m_instances.size(); ++i) {
        // First process inputs, plugging in either constants or outputs from other VGens as necessary.
        std::vector<std::string> vgenInputs;
        for (auto j = 0; j < m_instances[i].numberOfInputs(); ++j) {
            float constantValue;
            int vgenIndex;
            int vgenOutput;
            // Inputs are either constants other vgen outputs. If a constant we simply supply the constant directly.
            if (m_instances[i].getInputConstantValue(j, constantValue)) {
                vgenInputs.push_back(fmt::format("{}f", constantValue));
            } else if (m_instances[i].getInputVGenIndex(j, vgenIndex, vgenOutput)) {
                // If a VGen index we use the output name of the VGen at that index.
                vgenInputs.push_back(nameForVGenOutput(vgenIndex, vgenOutput));
            } else {
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
    }

    return true;
}

bool AbstractScinthDef::buildManifests() {
    // At minimum the vertex manifest must have the position data from the associated Shape.
    m_vertexManifest.addElement(m_prefix + "_inPosition", m_shape->elementType());

    // Other Intrinsics have manifest dependencies, process each in turn.
    for (Intrinsic intrinsic : m_intrinsics) {
        switch (intrinsic) {
        case kNormPos:
            m_vertexManifest.addElement(m_prefix + "_normPos", Manifest::ElementType::kVec2);
            break;

        case kTime:
            m_uniformManifest.addElement("time", Manifest::ElementType::kFloat);
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

    std::string positionName = m_prefix + "_inPosition";
    // Now produce the vertex shader outputs. TODO: allowing vertex shader VGens will require more data here. But for
    // now we just copy everything to the fragment shader except for the _inPosition, which gets assigned to the
    // keyword gl_Position.
    for (auto i = 0; i < m_vertexManifest.numberOfElements(); ++i) {
        if (m_vertexManifest.nameForElement(i) == positionName) {
            continue;
        }
        m_vertexShader += fmt::format("layout(location = {}) out {} out_{};\n", i,
                                      m_vertexManifest.typeNameForElement(i), m_vertexManifest.nameForElement(i));
    }

    // TODO: uniform is fragment-only for now.

    m_vertexShader += "\n"
                      "void main() {\n";

    // Assign all input elements to their respective output elements.
    for (auto i = 0; i < m_vertexManifest.numberOfElements(); ++i) {
        if (m_vertexManifest.nameForElement(i) == positionName) {
            switch (m_vertexManifest.typeForElement(i)) {
            case Manifest::ElementType::kFloat:
                m_vertexShader += fmt::format("    gl_Position = vec4({}, 0.0f, 0.0f, 1.0f);\n", positionName);
                break;

            case Manifest::ElementType::kVec2:
                m_vertexShader += fmt::format("    gl_Position = vec4({}, 0.0f, 1.0f);\n", positionName);
                break;

            case Manifest::ElementType::kVec3:
                m_vertexShader += fmt::format("    gl_Position = vec4({}, 1.0f);\n", positionName);
                break;

            case Manifest::ElementType::kVec4:
                m_vertexShader += fmt::format("    gl_Position = {};\n", positionName);
                break;
            }
        } else {
            m_vertexShader += fmt::format("    out_{} = in_{};\n", m_vertexManifest.nameForElement(i),
                                          m_vertexManifest.nameForElement(i));
        }
    }

    m_vertexShader += "}\n";
    return true;
}

bool AbstractScinthDef::buildFragmentShader() {
    // For now, all intrinsics are global, so we can define a single map with all of their substitutions.
    std::unordered_map<Intrinsic, std::string> intrinsicNames;
    for (Intrinsic intrinsic : m_intrinsics) {
        switch (intrinsic) {
        case kNormPos:
            intrinsicNames.insert({ kNormPos, "normPos" }); // TODO: has a different name in the vertex shader
            break;

        case kTime:
            intrinsicNames.insert({ kTime, m_prefix + "_ubo.time" });
            break;

        default:
            spdlog::error("Unknown intrinsic in AbstractScinthDesc {}", m_name);
            return false;
        }
    }

    // TODO: very hard-coded right now, need to make manifest-driven.
    // Now construct a fragment shader from the parameters.
    m_fragmentShader = "#version 450\n"
                       "#extension GL_ARB_separate_shader_objects : enable\n\n";

    if (m_intrinsics.count(Intrinsic::kNormPos)) {
        m_fragmentShader += "layout(location = 0) in vec2 normPos;\n\n";
    }

    if (m_intrinsics.count(Intrinsic::kTime)) {
        m_fragmentShader += "layout(binding = 0) uniform UBO {\n"
                            "  float time;\n"
                            "} "
            + m_prefix + "_ubo;\n\n";
    }

    for (auto i = 0; i < m_instances.size(); ++i) {
        m_fragmentShader += "\n\n// ------- " + m_instances[i].abstractVGen()->name() + "\n\n";
        m_fragmentShader += m_instances[i].abstractVGen()->parameterize(m_inputs[i], intrinsicNames, m_outputs[i]);
    }

    spdlog::info("fragshader: {}\n", m_fragmentShader);
    return true;
}

} // namespace scin
