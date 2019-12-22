#include "core/VGen.hpp"

#include "core/AbstractVGen.hpp"

#include "spdlog/spdlog.h"

namespace scin {

VGen::VGen(std::shared_ptr<const AbstractVGen> abstractVGen): m_abstractVGen(abstractVGen) {}

VGen::~VGen() {}

void VGen::addConstantInput(float constantValue) { m_inputs.emplace_back(VGenInput(constantValue)); }

void VGen::addVGenInput(int index) { m_inputs.emplace_back(VGenInput(index)); }

bool VGen::validate() {
    if (m_inputs.size() != m_abstractVGen->inputs().size()) {
        spdlog::error("input size mismatch for VGen {}, expected {}, got {}", m_abstractVGen->name(),
                      m_abstractVGen->inputs().size(), m_inputs.size());
        return false;
    }

    return true;
}

bool VGen::getInputConstantValue(int index, float& outValue) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == VGenInput::kConstant) {
            outValue = m_inputs[index].value.constant;
            return true;
        }
    }
    return false;
}

bool VGen::getInputVGenIndex(int index, int& outIndex) const {
    if (index >= 0 && index < numberOfInputs()) {
        if (m_inputs[index].type == VGenInput::kVGen) {
            outIndex = m_inputs[index].value.vgenIndex;
            return true;
        }
    }
    return false;
}

} // namespace scin
