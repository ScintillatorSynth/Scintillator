#ifndef SRC_VGEN_HPP_
#define SRC_VGEN_HPP_

#include <string>
#include <vector>

namespace scin {

class VGen {
public:
    VGen(const std::string& name, const std::string& fragment,
        const std::vector<std::string>& inputs = std::vector<std::string>(),
        const std::vector<std::string>& parameters = std::vector<std::string>(),
        const std::vector<std::string>& intermediates = std::vector<std::string>());
    ~VGen();

    const std::string& name() const { return m_name; }
    const std::string& fragment() const { return m_fragment; }
    const std::vector<std::string>& inputs() const { return m_inputs; }
    const std::vector<std::string>& parameters() const { return m_parameters; }
    const std::vector<std::string>& intermediates() const { return m_intermediates; }

private:
    std::string m_name;
    std::string m_fragment;
    std::vector<std::string> m_inputs;
    std::vector<std::string> m_parameters;
    std::vector<std::string> m_intermediates;
};

}  // namespace scin

#endif  // SRC_VGEN_HPP_

