#include "base/VGen.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace base {

VGen::VGen(std::shared_ptr<const AbstractVGen> abstractVGen, AbstractVGen::Rates rate):
    m_abstractVGen(abstractVGen),
    m_rate(rate) {}

VGen::~VGen() {}

void VGen::setSamplerConfig(size_t imageIndex, InputType imageArgType, const AbstractSampler& sampler) {
    m_imageIndex = imageIndex;
    m_imageArgType = imageArgType;
    m_abstractSampler = sampler;
}

void VGen::addConstantInput(float constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }
void VGen::addConstantInput(glm::vec2 constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }
void VGen::addConstantInput(glm::vec3 constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }
void VGen::addConstantInput(glm::vec4 constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }

void VGen::addParameterInput(size_t parameterIndex) { m_inputs.emplace_back(VGenInput(parameterIndex)); }

void VGen::addVGenInput(size_t vgenIndex, size_t outputIndex, size_t dimension) {
    m_inputs.emplace_back(VGenInput(vgenIndex, outputIndex, dimension));
}

void VGen::addOutput(size_t dimension) { m_outputDimensions.push_back(dimension); }

bool VGen::validate() const {
    if (m_inputs.size() != m_abstractVGen->inputs().size()) {
        spdlog::error("input size mismatch for VGen {}, expected {}, got {}", m_abstractVGen->name(),
                      m_abstractVGen->inputs().size(), m_inputs.size());
        return false;
    }

    if ((m_rate & m_abstractVGen->supportedRates()) == 0) {
        spdlog::error("Unsupported rate for VGen {}", m_abstractVGen->name());
        return false;
    }

    // TODO: check input and output dimensions.

    return true;
}

VGen::InputType VGen::getInputType(size_t index) const {
    if (index < numberOfInputs()) {
        return m_inputs[index].type;
    }
    return kInvalid;
}

bool VGen::getInputConstantValue(size_t index, float& outValue) const {
    if (index < numberOfInputs()) {
        if (m_inputs[index].type == kConstant && m_inputs[index].dimension == 1) {
            outValue = m_inputs[index].value.constant1;
            return true;
        }
    }
    return false;
}

bool VGen::getInputConstantValue(size_t index, glm::vec2& outValue) const {
    if (index < numberOfInputs()) {
        if (m_inputs[index].type == kConstant && m_inputs[index].dimension == 2) {
            outValue = m_inputs[index].value.constant2;
            return true;
        }
    }
    return false;
}

bool VGen::getInputConstantValue(size_t index, glm::vec3& outValue) const {
    if (index < numberOfInputs()) {
        if (m_inputs[index].type == kConstant && m_inputs[index].dimension == 3) {
            outValue = m_inputs[index].value.constant3;
            return true;
        }
    }
    return false;
}

bool VGen::getInputConstantValue(size_t index, glm::vec4& outValue) const {
    if (index < numberOfInputs()) {
        if (m_inputs[index].type == kConstant && m_inputs[index].dimension == 4) {
            outValue = m_inputs[index].value.constant4;
            return true;
        }
    }
    return false;
}

bool VGen::getInputParameterIndex(size_t index, size_t& outIndex) const {
    if (index < numberOfInputs()) {
        if (m_inputs[index].type == kParameter) {
            outIndex = m_inputs[index].value.parameterIndex;
            return true;
        }
    }
    return false;
}

bool VGen::getInputVGenIndex(size_t index, size_t& outIndex, size_t& outOutput) const {
    if (index < numberOfInputs()) {
        if (m_inputs[index].type == kVGen) {
            outIndex = m_inputs[index].value.vgenIndex;
            outOutput = m_inputs[index].vgenOutputIndex;
            return true;
        }
    }
    return false;
}

} // namespace base

} // namespace scin
