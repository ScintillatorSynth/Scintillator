#include "VGenInstance.hpp"

#include "VGen.hpp"

#include "spdlog/spdlog.h"

namespace scin {

VGenInstance::VGenInstance(std::shared_ptr<const VGen> vgen): m_vgen(vgen) {}

VGenInstance::~VGenInstance() {}

void VGenInstance::addConstantInput(float constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }

void VGenInstance::addVGenInput(int index) { m_inputs.emplace_back(VGenInput(index)); }

bool VGenInstance::validate() {
    if (m_inputs.size() != m_vgen->inputs().size()) {
        spdlog::warn("input size mismatch for VGen {}, expected {}, got {}", m_vgen->name(), m_vgen->inputs().size(),
                     m_inputs.size());
        return false;
    }

    return true;
}

bool VGenInstance::getInputConstantValue(int index, float& outValue) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == VGenInput::kConstant) {
            outValue = m_inputs[index].value.constant;
            return true;
        }
    }
    return false;
}

bool VGenInstance::getInputVGenIndex(int index, int& outIndex) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == VGenInput::kVGen) {
            outIndex = m_inputs[index].value.vgenIndex;
            return true;
        }
    }
    return false;
}

} // namespace scin
