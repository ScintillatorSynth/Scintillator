#include "VGen.hpp"

namespace scin {

VGen::VGen(const std::string& name, const std::string& fragment, const std::vector<std::string>& inputs,
    const std::vector<std::string>& parameters, const std::vector<std::string>& intermediates) :
    m_name(name),
    m_fragment(fragment),
    m_inputs(inputs),
    m_parameters(parameters),
    m_intermediates(intermediates) {
}

VGen::~VGen() {
}

}  // namespace scin

