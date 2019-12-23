#include "core/AbstractVGen.hpp"

#include "spdlog/spdlog.h"

#include <unordered_map>

namespace scin {

AbstractVGen::AbstractVGen(const std::string& name, const std::string& fragment, const std::vector<std::string>& inputs,
                           const std::vector<std::string>& parameters, const std::vector<std::string>& intermediates):
    m_name(name),
    m_fragment(fragment),
    m_inputs(inputs),
    m_parameters(parameters),
    m_intermediates(intermediates),
    m_valid(false) {}

AbstractVGen::~AbstractVGen() {}

bool AbstractVGen::prepareTemplate() {
    // First build a map of all parameters (also verifying uniqueness of names in the process)
    std::unordered_map<std::string, Parameter> parameterMap;
    for (auto i = 0; i < m_inputs.size(); ++i) {
        if (parameterMap.find(m_inputs[i]) != parameterMap.end()) {
            spdlog::error("VGen {} has a duplicate parameter name {}", m_name, m_inputs[i]);
            return false;
        }
        parameterMap.insert({ m_inputs[i], Parameter(Parameter::Kind::kInput, i) });
    }
    for (auto i = 0; i < m_parameters.size(); ++i) {
        if (parameterMap.find(m_parameters[i]) != parameterMap.end()) {
            spdlog::error("VGen {} has a duplicate parameter name {}", m_name, m_parameters[i]);
            return false;
        }
        parameterMap.insert({ m_parameters[i], Parameter(Parameter::Kind::kIntrinsic, i) });
    }
    for (auto i = 0; i < m_intermediates.size(); ++i) {
        if (parameterMap.find(m_intermediates[i]) != parameterMap.end()) {
            spdlog::error("VGen {} has a duplicate parameter name {}", m_name, m_parameters[i]);
            return false;
        }
        parameterMap.insert({ m_intermediates[i], Parameter(Parameter::Kind::kIntermediate, i) });
    }
    // @out is not a valid shader parameter name.
    if (parameterMap.find("out") != parameterMap.end()) {
        spdlog::error("VGen {} using reserved parameter name @out.", m_name);
        return false;
    }
    parameterMap.insert({ "out", Parameter(Parameter::Kind::kOut, 0) });

    m_fragmentParameters.clear();
    // The @out parameter should appear exactly once in the fragment.
    int outCount = 0;

    // Looking for a single @ symbol followed by word characters (A-Za-z0-9_-) until whitespace.
    std::regex regex("@{1}\\w+");
    for (auto i = std::sregex_iterator(m_fragment.begin(), m_fragment.end(), regex); i != std::sregex_iterator(); ++i) {
        // Each parameter identified with the @ prefix should have a match in the map.
        auto param = parameterMap.find(i->str().substr(1));
        if (param == parameterMap.end()) {
            spdlog::error("VGen {} parsed unidentified parameter {} at position {} in shader '{}'", m_name, i->str(),
                          i->position(), m_fragment);
            return false;
        }
        if (param->second.kind == Parameter::Kind::kOut) {
            ++outCount;
        }
        m_fragmentParameters.push_back({ *i, param->second });
    }

    if (outCount != 1) {
        spdlog::error("VGen {}: @out parameter must appear exactly once in shader '{}'", m_name, m_fragment);
        return false;
    }

    m_valid = true;
    return true;
}

} // namespace scin
