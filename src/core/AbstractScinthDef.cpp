#include "core/AbstractScinthDef.hpp"

#include "core/AbstractVGen.hpp"
#include "core/VGen.hpp"

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
    std::array<char, 256> buffer;

    // Build the parameters for all VGens.
    m_uniforms.clear();
    m_inputs.clear();
    m_intrinsics.clear();
    m_intermediates.clear();
    int intermediatesCount = 0;

    for (auto i = 0; i < m_instances.size(); ++i) {
        // First process inputs, plugging in either constants or outputs from other VGens as necessary.
        std::vector<std::string> vgenInputs;
        for (auto j = 0; j < m_instances[i].numberOfInputs(); ++j) {
            float constantValue;
            int vgenIndex;
            // Inputs are either constants other vgen outputs. If a constant we simply supply the constant directly.
            if (m_instances[i].getInputConstantValue(j, constantValue)) {
                std::snprintf(buffer.data(), sizeof(buffer), "%ff", constantValue);
                vgenInputs.push_back(std::string(buffer.data()));
            } else if (m_instances[i].getInputVGenIndex(j, vgenIndex)) {
                // If a VGen index we use the output name of the VGen at that index.
                vgenInputs.push_back(nameForVGenOutput(vgenIndex));
            } else {
                spdlog::error("AbstractScinthDesc {} VGen at index {} has unknown input type at index {}.", m_name, i,
                              j);
                return false;
            }
        }
        m_inputs.push_back(vgenInputs);

        // Collect all intrinsics for provision at runtime by the Scinth.
        std::vector<std::string> vgenIntrinsics;
        for (auto intrinsic : m_instances[i].abstractVGen()->intrinsics()) {
            // TODO: likely there will be other ways to provide data than uniforms? Certainly some things will come from
            // vertex shaders.
            m_uniforms.insert(intrinsic);
            vgenIntrinsics.push_back(intrinsic);
        }
        m_intrinsics.push_back(vgenIntrinsics);

        // Intermediate names just increment a counter to keep them unique.
        std::vector<std::string> vgenIntermediates;
        for (auto intermediate : m_instances[i].abstractVGen()->intermediates()) {
            std::snprintf(buffer.data(), sizeof(buffer), "%s_temp_%d", m_uniquePrefix.data(), intermediatesCount);
            vgenIntermediates.push_back(std::string(buffer.data()));
            ++intermediatesCount;
        }
        m_intermediates.push_back(vgenIntermediates);
    }

    // Now construct a fragment shader from the parameters.
    m_fragmentShader.clear();
    for (auto i = 0; i < m_instances.size(); ++i) {
        m_fragmentShader += "\n\n// ------- " + m_instances[i].abstractVGen()->name() + "\n\n";
        m_fragmentShader += m_instances[i].abstractVGen()->parameterize(m_inputs[i], m_intrinsics[i],
                                                                        m_intermediates[i], nameForVGenOutput(i));
    }

    spdlog::info("fragshader: {}\n", m_fragmentShader);

    return true;
}

std::string AbstractScinthDef::nameForVGenOutput(int index) const {
    if (index < 0 || index >= m_instances.size()) {
        return std::string("");
    }
    // TODO: fix hard-coded assumption here about gl_FragColor
    if (index == m_instances.size() - 1) {
        return std::string("gl_FragColor");
    }
    std::array<char, 256> buffer;
    std::snprintf(buffer.data(), sizeof(buffer), "%s_out_%d", m_uniquePrefix.data(), index);
    return std::string(buffer.data());
}

} // namespace scin
