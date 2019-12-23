#ifndef SRC_CORE_ABSTRACT_VGEN_HPP_
#define SRC_CORE_ABSTRACT_VGEN_HPP_

#include <regex>
#include <string>
#include <vector>

namespace scin {

/*! Represents a template for a parameterized shader program suitable for composition in a VGen graph.
 *
 *  This class parses incoming VGen definitions and prepares them for efficient processing for shader program
 *  generation.
 */
class AbstractVGen {
public:
    AbstractVGen(const std::string& name, const std::string& fragment,
                 const std::vector<std::string>& inputs = std::vector<std::string>(),
                 const std::vector<std::string>& parameters =
                     std::vector<std::string>(), // TODO: unfortunate name, consider intrinsic
                 const std::vector<std::string>& intermediates = std::vector<std::string>());
    ~AbstractVGen();

    /*! Evaluate the provided shader and parameters, prepare for requests for parameterization.
     *
     * \return true if this AbstractVGen is well-formed, false if it is not.
     */
    bool prepareTemplate();

    const std::string& name() const { return m_name; }
    const std::string& fragment() const { return m_fragment; }
    const std::vector<std::string>& inputs() const { return m_inputs; }
    const std::vector<std::string>& parameters() const { return m_parameters; }
    const std::vector<std::string>& intermediates() const { return m_intermediates; }
    bool valid() const { return m_valid; }

private:
    struct Parameter {
        enum Kind { kInput, kIntrinsic, kIntermediate, kOut };
        Parameter(Kind k, size_t i): kind(k), index(i) {}
        Kind kind;
        size_t index;
    };

    std::string m_name;
    std::string m_fragment;
    std::vector<std::string> m_inputs;
    std::vector<std::string> m_parameters;
    std::vector<std::string> m_intermediates;

    bool m_valid;
    std::vector<std::pair<const std::smatch, Parameter>> m_fragmentParameters;
};

} // namespace scin

#endif // SRC_CORE_ABSTRACT_VGEN_HPP_
