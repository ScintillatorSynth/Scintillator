#ifndef SRC_CORE_ABSTRACT_VGEN_HPP_
#define SRC_CORE_ABSTRACT_VGEN_HPP_

#include "base/Intrinsic.hpp"

#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace scin { namespace base {

/*! Represents a template for a parameterizable shader program suitable for composition in a VGen graph.
 *
 *  This class parses incoming VGen definitions and prepares them for efficient processing for shader program
 *  generation.
 */
class AbstractVGen {
public:
    /*! Bitflags to indicate supported rates for this VGen.
     */
    enum Rates : unsigned { kNone = 0, kFrame = 1, kShape = 2, kPixel = 4 };

    /*! Construct an AbstractVGen with all required and optional data.
     *
     * \param name The name to use for this VGen, must be unique.
     * \param isSampler If true indicates this is a sampling VGen
     * \param inputs A list of input names, can be empty.
     * \param outputs A list of output names, must be at least one.
     * \param inputDimensions Each subarray describes the allowable dimensions of the input at that index, and so should
     *        have the same number of entries as the number of inputs to the VGen. There should be the same number of
     *        subarrays in inputDimensions as there are in outputDimensions, and the values at each index of both arrays
     *        form a pair of input/output dimensions. In the event the VGen has no inputs there should be one empty
     *        subarray for each output dimension entry.
     * \param outputDimensions Describes the dimensionality of each output in the input/output dimension pairs formed
     *        with inputDimesion.
     * \param shader The template shader code.
     */
    AbstractVGen(const std::string& name, unsigned supportedRates, bool isSampler,
                 const std::vector<std::string>& inputs, const std::vector<std::string>& outputs,
                 const std::vector<std::vector<size_t>> inputDimensions,
                 const std::vector<std::vector<size_t>> outputDimensions, const std::string& shader);
    ~AbstractVGen();

    /*! Evaluate the provided shader and parameters, prepare for requests for parameterization.
     *
     * \note If this AbstractVGen is not valid, further use of it will result in undefined behavior.
     * \return true if this AbstractVGen is well-formed, false if it is not.
     */
    bool prepareTemplate();

    /*! Build and return a shader source string with the supplied names substituted for the parameters.
     *
     *  \param inputs The list of input names to substitute in the shader.
     *  \param intrinsics A map of intrinsic names to substitute in the shader.
     *  \param intermediates The list of intermediate names to substitute in the shader.
     *  \param outputs The names of the output variables to substitute in the shader.
     *  \param outputDimensions The dimensions of the outputs, for defining new variables for the outputs.
     *  \param alreadyDefined Some output variables may already be defined, include them here to avoid redefinition.
     *  \return The substituted string, or an empty string on error or invalid AbstractVGen.
     */
    std::string parameterize(const std::vector<std::string>& inputs,
                             const std::unordered_map<Intrinsic, std::string>& intrinsics,
                             const std::vector<std::string>& outputs, const std::vector<size_t>& outputDimensions,
                             const std::unordered_set<std::string>& alreadyDefined) const;

    const std::string& name() const { return m_name; }
    unsigned supportedRates() const { return m_supportedRates; }
    bool isSampler() const { return m_isSampler; }
    const std::vector<std::string>& inputs() const { return m_inputs; }
    const std::unordered_set<Intrinsic>& intrinsics() const { return m_intrinsics; }
    const std::vector<std::string>& outputs() const { return m_outputs; }
    const std::vector<std::vector<size_t>>& inputDimensions() const { return m_inputDimensions; }
    const std::vector<std::vector<size_t>>& outputDimensions() const { return m_outputDimensions; }
    const std::string& shader() const { return m_shader; }
    bool valid() const { return m_valid; }

private:
    struct Parameter {
        enum Kind { kInput, kIntrinsic, kOutput };
        explicit Parameter(Kind k, size_t i): kind(k) { value.index = i; }
        explicit Parameter(Intrinsic intr): kind(kIntrinsic) { value.intrinsic = intr; }
        Kind kind;
        union Value {
            Intrinsic intrinsic;
            size_t index;
        };
        Value value;
    };

    std::string m_name;
    unsigned m_supportedRates;
    bool m_isSampler;
    std::vector<std::string> m_inputs;
    std::vector<std::string> m_outputs;
    std::vector<std::vector<size_t>> m_inputDimensions;
    std::vector<std::vector<size_t>> m_outputDimensions;
    std::string m_shader;

    bool m_valid;
    std::unordered_set<Intrinsic> m_intrinsics;
    std::vector<std::pair<const std::smatch, Parameter>> m_parameters;
};

} // namespace base

} // namespace scin

#endif // SRC_CORE_ABSTRACT_VGEN_HPP_
