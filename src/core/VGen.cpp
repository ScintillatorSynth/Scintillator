#include "core/VGen.hpp"

#include "core/AbstractVGen.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace core {

VGen::VGen(std::shared_ptr<const AbstractVGen> abstractVGen): m_abstractVGen(abstractVGen) {}

VGen::~VGen() {}

void VGen::setSamplerConfig(int imageIndex, InputType imageArgType, const AbstractSampler& sampler) {
    m_imageIndex = imageIndex;
    m_imageArgType = imageArgType;
    m_abstractSampler = sampler;
}

void VGen::addConstantInput(float constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }
void VGen::addConstantInput(glm::vec2 constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }
void VGen::addConstantInput(glm::vec3 constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }
void VGen::addConstantInput(glm::vec4 constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }

void VGen::addParameterInput(int parameterIndex) { m_inputs.emplace_back(VGenInput(parameterIndex)); }

void VGen::addVGenInput(int vgenIndex, int outputIndex, int dimension) {
    m_inputs.emplace_back(VGenInput(vgenIndex, outputIndex, dimension));
}

void VGen::addOutput(int dimension) { m_outputDimensions.push_back(dimension); }

bool VGen::validate() const {
    if (m_inputs.size() != m_abstractVGen->inputs().size()) {
        spdlog::error("input size mismatch for VGen {}, expected {}, got {}", m_abstractVGen->name(),
                      m_abstractVGen->inputs().size(), m_inputs.size());
        return false;
    }

    // TODO: check input and output dimensions.

    return true;
}

VGen::InputType VGen::getInputType(int index) const {
    if (index >= 0 && index < numberOfInputs()) {
        return m_inputs[index].type;
    }
    return kInvalid;
}

bool VGen::getInputConstantValue(int index, float& outValue) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == kConstant && m_inputs[index].dimension == 1) {
            outValue = m_inputs[index].value.constant1;
            return true;
        }
    }
    return false;
}

bool VGen::getInputConstantValue(int index, glm::vec2& outValue) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == kConstant && m_inputs[index].dimension == 2) {
            outValue = m_inputs[index].value.constant2;
            return true;
        }
    }
    return false;
}

bool VGen::getInputConstantValue(int index, glm::vec3& outValue) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == kConstant && m_inputs[index].dimension == 3) {
            outValue = m_inputs[index].value.constant3;
            return true;
        }
    }
    return false;
}

bool VGen::getInputConstantValue(int index, glm::vec4& outValue) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == kConstant && m_inputs[index].dimension == 4) {
            outValue = m_inputs[index].value.constant4;
            return true;
        }
    }
    return false;
}

bool VGen::getInputParameterIndex(int index, int& outIndex) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == kParameter) {
            outIndex = m_inputs[index].value.parameterIndex;
            return true;
        }
    }
    return false;
}

bool VGen::getInputVGenIndex(int index, int& outIndex, int& outOutput) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == kVGen) {
            outIndex = m_inputs[index].value.vgen.x;
            outOutput = m_inputs[index].value.vgen.y;
            return true;
        }
    }
    return false;
}

} // namespace core

} // namespace scin
