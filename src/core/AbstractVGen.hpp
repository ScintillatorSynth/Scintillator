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
    // TODO: intrinsics could be inferred, as they are reserved words and therefore can't be used as input or
    // intermediate names, and will then be the only @ tags in the fragment without a presence in the list.
    AbstractVGen(const std::string& name, const std::string& fragment,
                 const std::vector<std::string>& inputs = std::vector<std::string>(),
                 const std::vector<std::string>& intrinsics = std::vector<std::string>(),
                 const std::vector<std::string>& intermediates = std::vector<std::string>());
    ~AbstractVGen();

    /*! Evaluate the provided shader and parameters, prepare for requests for parameterization.
     *
     * \return true if this AbstractVGen is well-formed, false if it is not.
     */
    bool prepareTemplate();

    /*! Build and return a fragment shader string with the supplied names substituted for the parameters.
     *
     *  \param inputs The list of input names to substitute in the fragment string.
     *  \param intrinsics The list of intrinsic names to substitute in the fragment string.
     *  \param intermediates The list of intermediate names to substitute in the fragment string.
     *  \param out The name of the output variable to substitute in the fragment string.
     *  \return The substituted string, or an empty string on error or invalid AbstractVGen.
     */
    std::string parameterize(const std::vector<std::string>& inputs, const std::vector<std::string>& intrinsics,
                             const std::vector<std::string>& intermediates, const std::string& out) const;

    const std::string& name() const { return m_name; }
    const std::string& fragment() const { return m_fragment; }
    const std::vector<std::string>& inputs() const { return m_inputs; }
    const std::vector<std::string>& intrinsics() const { return m_intrinsics; }
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
    std::vector<std::string> m_intrinsics;
    std::vector<std::string> m_intermediates;

    bool m_valid;
    std::vector<std::pair<const std::smatch, Parameter>> m_fragmentParameters;
};

} // namespace scin

#endif // SRC_CORE_ABSTRACT_VGEN_HPP_
