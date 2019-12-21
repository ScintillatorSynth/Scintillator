#include "VGenInstance.hpp"

#include "VGen.hpp"

#include "spdlog/spdlog.h"

namespace scin {

VGenInstance::VGenInstance(std::shared_ptr<VGen> vgen): m_vgen(vgen) {}

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

} // namespace scin
