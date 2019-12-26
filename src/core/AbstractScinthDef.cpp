#include "core/AbstractScinthDef.hpp"

#include "core/AbstractVGen.hpp"
#include "core/VGen.hpp"

#include "glm/glm.hpp"
// TODO: can you reuse the fmt code from spdlog here?
#include "spdlog/spdlog.h"

#include <array>
#include <cstdio>
#include <random>

namespace scin {

AbstractScinthDef::AbstractScinthDef(const std::string& name, const std::vector<VGen>& instances):
    m_name(name),
    m_instances(instances) {
    std::random_device randomDevice;
    std::array<char, 256> buffer;
    std::snprintf(buffer.data(), sizeof(buffer), "%s_%08x", m_name.data(), randomDevice());
    m_uniquePrefix = std::string(buffer.data());
}

AbstractScinthDef::~AbstractScinthDef() {}

bool AbstractScinthDef::build() {
    if (!buildParameters()) {
        return false;
    }
    if (!buildShaders()) {
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
    std::array<char, 256> buffer;
    std::snprintf(buffer.data(), sizeof(buffer), "%s_out_%d_%d", m_uniquePrefix.data(), vgenIndex, outputIndex);
    return std::string(buffer.data());
}

bool AbstractScinthDef::buildParameters() {
    std::array<char, 256> buffer;

    // Build the parameters for all VGens.
    m_intrinsics.clear();
    m_inputs.clear();
    m_outputs.clear();
    int intermediatesCount = 0;

    for (auto i = 0; i < m_instances.size(); ++i) {
        // First process inputs, plugging in either constants or outputs from other VGens as necessary.
        std::vector<std::string> vgenInputs;
        for (auto j = 0; j < m_instances[i].numberOfInputs(); ++j) {
            float constantValue;
            int vgenIndex;
            int vgenOutput;
            // Inputs are either constants other vgen outputs. If a constant we simply supply the constant directly.
            if (m_instances[i].getInputConstantValue(j, constantValue)) {
                std::snprintf(buffer.data(), sizeof(buffer), "%ff", constantValue);
                vgenInputs.push_back(std::string(buffer.data()));
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

bool AbstractScinthDef::buildShaders() {
    // There's only two flavors of vertex shaders right now, one with the normPos and on without.
    if (m_intrinsics.count(Intrinsic::kNormPos)) {
        m_vertexShader = std::string("vertex shader",
                                     "#version 450\n"
                                     "#extension GL_ARB_separate_shader_objects : enable\n"
                                     "\n"
                                     "layout(location = 0) in vec2 inPosition;\n"
                                     "layout(location = 1) in vec2 inNormPosition;\n"
                                     "\n"
                                     "layout(location = 0) out vec2 normPos;\n"
                                     "\n"
                                     "void main() {\n"
                                     "  gl_Position = vec4(inPosition, 0.0, 1.0);\n"
                                     "  normPos = inNormPosition;\n"
                                     "}\n");
    } else {
        m_vertexShader = std::string("vertex shader",
                                     "#version 450\n"
                                     "#extension GL_ARB_separate_shader_objects : enable\n"
                                     "\n"
                                     "layout(location = 0) in vec2 inPosition;\n"
                                     "\n"
                                     "layout(location = 0) out vec2 normPos;\n"
                                     "\n"
                                     "void main() {\n"
                                     "  gl_Position = vec4(inPosition, 0.0, 1.0);\n"
                                     "}\n");
    }

    // For now, all intrinsics are global, so we can define a single map with all of their substitutions.
    std::unordered_map<Intrinsic, std::string> intrinsics;
    for (Intrinsic intrinsic : m_intrinsics) {
        switch (intrinsic) {
        case kNormPos:
            intrinsics.insert({ kNormPos, "normPos" }); // TODO: has a different name in the vertex shader
            break;

        case kTime:
            intrinsics.insert({ kTime, m_uniquePrefix + "_ubo.time" });
            break;

        default:
            spdlog::error("Unknown intrinsic in AbstractScinthDesc {}", m_name);
            return false;
        }
    }

    // Now construct a fragment shader from the parameters.
    m_fragmentShader = "#version 450\n"
                       "#extension GL_ARB_separate_shader_objects : enable\n\n";
    if (m_intrinsics.count(Intrinsic::kTime)) {
        m_fragmentShader += "layout(binding = 0) uniform UBO {\n"
                            "  float time;\n"
                            "} "
            + m_uniquePrefix + "_ubo;\n\n";
    }

    if (m_intrinsics.count(Intrinsic::kNormPos)) {
        m_fragmentShader += "layout(location = 0) in vec2 normPos;\n\n";
    }

    for (auto i = 0; i < m_instances.size(); ++i) {
        m_fragmentShader += "\n\n// ------- " + m_instances[i].abstractVGen()->name() + "\n\n";
        m_fragmentShader += m_instances[i].abstractVGen()->parameterize(m_inputs[i], intrinsics, m_outputs[i]);
    }

    spdlog::info("fragshader: {}\n", m_fragmentShader);
    return true;
}

} // namespace scin
