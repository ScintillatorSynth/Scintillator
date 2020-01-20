#include "core/AbstractVGen.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace core {

AbstractVGen::AbstractVGen(const std::string& name, const std::vector<std::string>& inputs,
                           const std::vector<std::string>& outputs, const std::vector<std::vector<int>> inputDimensions,
                           const std::vector<std::vector<int>> outputDimensions, const std::string& shader):
    m_name(name),
    m_inputs(inputs),
    m_outputs(outputs),
    m_inputDimensions(inputDimensions),
    m_outputDimensions(outputDimensions),
    m_shader(shader),
    m_valid(false) {}

AbstractVGen::~AbstractVGen() {}

bool AbstractVGen::prepareTemplate() {
    // First build a map of all tokens (also verifying uniqueness of names in the process)
    std::unordered_map<std::string, Parameter> parameterMap;
    for (auto i = 0; i < m_inputs.size(); ++i) {
        if (parameterMap.find(m_inputs[i]) != parameterMap.end()) {
            spdlog::error("VGen {} has a duplicate parameter name {}", m_name, m_inputs[i]);
            return false;
        }
        if (getIntrinsicNamed(m_inputs[i]) != Intrinsic::kNotFound) {
            spdlog::error("VGen {} has reserved intrinsic name {} as input", m_name, m_inputs[i]);
            return false;
        }
        parameterMap.insert({ m_inputs[i], Parameter(Parameter::Kind::kInput, i) });
    }
    for (auto i = 0; i < m_outputs.size(); ++i) {
        if (parameterMap.find(m_outputs[i]) != parameterMap.end()) {
            spdlog::error("VGen {} has a duplicate parameter name {}", m_name, m_outputs[i]);
            return false;
        }
        if (getIntrinsicNamed(m_outputs[i]) != Intrinsic::kNotFound) {
            spdlog::error("VGen {} has reserved intrinsic name {} as output", m_name, m_outputs[i]);
            return false;
        }
        parameterMap.insert({ m_outputs[i], Parameter(Parameter::Kind::kOutput, i) });
    }

    // There should be at least one mention of an output parameter in the shader.
    bool outFound = false;

    // Looking for a single @ symbol followed by word characters (A-Za-z0-9_-) until whitespace.
    std::regex regex("@{1}\\w+");
    for (auto i = std::sregex_iterator(m_shader.begin(), m_shader.end(), regex); i != std::sregex_iterator(); ++i) {
        // Each parameter identified with the @ prefix should have a match in the map or be an intrinsic.
        auto param = parameterMap.find(i->str().substr(1));
        if (param != parameterMap.end()) {
            if (param->second.kind == Parameter::Kind::kOutput) {
                outFound = true;
            }
            m_parameters.push_back({ *i, param->second });
        } else {
            // Parameter could be an intrinsic, check there.
            Intrinsic intrinsic = getIntrinsicNamed(i->str().substr(1));
            if (intrinsic != Intrinsic::kNotFound) {
                m_parameters.push_back({ *i, Parameter(intrinsic) });
                m_intrinsics.insert(intrinsic);
            } else {
                spdlog::error("VGen {} parsed unidentified parameter {} at position {} in shader '{}'", m_name,
                              i->str(), i->position(), m_shader);
                return false;
            }
        }
    }

    if (!outFound) {
        spdlog::error("VGen {}: some out parameter must appear at least once in shader '{}'", m_name, m_shader);
        return false;
    }

    m_valid = true;
    return true;
}

std::string AbstractVGen::parameterize(const std::vector<std::string>& inputs,
                                       const std::unordered_map<Intrinsic, std::string>& intrinsics,
                                       const std::vector<std::string>& outputs,
                                       const std::vector<int>& outputDimensions,
                                       const std::unordered_set<std::string>& alreadyDefined) const {
    if (!m_valid) {
        spdlog::error("VGen {} parameterized but invalid.", m_name);
        return "";
    }
    if (inputs.size() != m_inputs.size() || outputs.size() != m_outputs.size()) {
        spdlog::error("VGen {} parameter count mismatch, expecting {} inputs got {}, expecting {} outputs got {}.",
                      m_name, m_inputs.size(), inputs.size(), m_intrinsics.size(), intrinsics.size(), m_outputs.size(),
                      outputs.size());
        return "";
    }

    std::string shader;
    size_t shaderPos = 0;
    // We keep a running list of the first time we encounter outputs in the shader code, because we will need to
    // declare them.
    std::unordered_set<int> outputsEncountered;
    for (auto param : m_parameters) {
        if (shaderPos < param.first.position()) {
            shader += m_shader.substr(shaderPos, param.first.position() - shaderPos);
            shaderPos = param.first.position();
        }
        switch (param.second.kind) {
        case Parameter::Kind::kInput:
            shader += inputs[param.second.value.index];
            break;

        case Parameter::Kind::kIntrinsic:
            shader += intrinsics.at(param.second.value.intrinsic);
            break;

        case Parameter::Kind::kOutput:
            if (outputsEncountered.count(param.second.value.index) == 0) {
                // We make an exception for any already defined variables, which don't need declaration.
                if (alreadyDefined.count(outputs[param.second.value.index]) == 0) {
                    switch (outputDimensions[param.second.value.index]) {
                    case 1:
                        shader += "float ";
                        break;

                    case 2:
                        shader += "vec2 ";
                        break;

                    case 3:
                        shader += "vec3 ";
                        break;

                    case 4:
                        shader += "vec4 ";
                        break;

                    default:
                        spdlog::error("unsupported dimension {} for output {} in VGen {}",
                                      outputDimensions[param.second.value.index], param.second.value.index, m_name);
                        return "";
                    }
                }
                outputsEncountered.insert(param.second.value.index);
            }
            shader += outputs[param.second.value.index];
            break;
        }
        // Advance string position past size of original parameter.
        shaderPos = shaderPos + param.first.length();
    }

    // Append any remaining unsubstituted shader code to the output.
    shader += m_shader.substr(shaderPos);

    return shader;
}

} // namespace core

} // namespace scin
