#ifndef SRC_BASE_VGEN_HPP_
#define SRC_BASE_VGEN_HPP_

#include "base/AbstractSampler.hpp"
#include "base/AbstractVGen.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace scin { namespace base {

/*! Represents a single node in the signal flow graph of a ScinthDef. Combines an AbstractVGen and inputs.
 */
class VGen {
public:
    VGen(std::shared_ptr<const AbstractVGen> abstractVGen, AbstractVGen::Rates rate);
    ~VGen();

    enum InputType { kConstant, kVGen, kParameter, kInvalid };

    /*! Sets the sampler configuration information for this VGen instance. This information will be ignored on
     * non-sampling VGens.
     */
    void setSamplerConfig(size_t imageIndex, InputType imageArgType, const AbstractSampler& sampler);

    /*! Sets the Tween index for the VGen, if needed.
     */
    void setTweenIndex(int tweenIndex);

    /*! Adds a single-dimensional constant-valued input.
     *
     * \param constantValue The value of the constant to supply to this input.
     */
    void addConstantInput(float constantValue);
    void addConstantInput(glm::vec2 constantValue);
    void addConstantInput(glm::vec3 constantValue);
    void addConstantInput(glm::vec4 constantValue);

    /*! Add a named ScinthDef parameter as input.
     *
     * \param parameterIndex The index of the parameter in the overall parameter list.
     */
    void addParameterInput(size_t parameterIndex);

    /*! Add output from another VGen as input.
     *
     * \note  These values are not validated as they require knowledge outside of this VGen to validate.
     * \param vgenIndex The index of the VGen within the ScinthDef that will provide input.
     * \param outputIndex The index of the output from the VGen use as input.
     * \param dimension The dimension of the input.
     */
    void addVGenInput(size_t vgenIndex, size_t outputIndex, size_t dimension);

    /*! Add an output to this VGen with supplied dimension.
     *
     * \param dimension The dimension of the output.
     */
    void addOutput(size_t dimension);

    /*! Check this instance data against the originating AbstractVGen.
     *
     * \return true if valid, false if not.
     */
    bool validate() const;

    /*! Return the type of input associated with the provided index.
     *
     * \param index The index of the input in question.
     * \return InputType an enumeration of the type of input, or kInvalid on error.
     */
    InputType getInputType(size_t index) const;

    /*! If the input at index is a constant, return true and store the constant value in outValue.
     *
     * \param index The index of the input in question.
     * \param outValue Will be set to the value of the constant on successful completion of the function.
     * \return true if the input at index is a constant, false otherwise.
     */
    bool getInputConstantValue(size_t index, float& outValue) const;
    bool getInputConstantValue(size_t index, glm::vec2& outValue) const;
    bool getInputConstantValue(size_t index, glm::vec3& outValue) const;
    bool getInputConstantValue(size_t index, glm::vec4& outValue) const;

    /*! If the input at index is a parameter, return true and store the parameter index value in outIndex.
     *
     * \param index The index of the input in question.
     * \param outIndex The index of the parameter input assoicated with this VGen input.
     * \return true if the input at index is a parameter, false otherwise.
     */
    bool getInputParameterIndex(size_t index, size_t& outIndex) const;

    /*! If the input at index is a VGen, return true and store the vgen index value in outIndex.
     *
     * \param index The index of the input in question.
     * \param outIndex Will be set to the value of the index of VGen's output on successful completion of the function.
     * \param outOutput Will be set to the value of the index of the output on the selected VGen.
     * \return true if the input at index is a VGen, false otherwise.
     */
    bool getInputVGenIndex(size_t index, size_t& outIndex, size_t& outOutput) const;

    size_t numberOfInputs() const { return m_inputs.size(); }

    size_t numberOfOutputs() const { return m_outputDimensions.size(); }
    size_t outputDimension(size_t index) const { return m_outputDimensions[index]; }

    std::shared_ptr<const AbstractVGen> abstractVGen() const { return m_abstractVGen; }
    AbstractVGen::Rates rate() const { return m_rate; }

    size_t imageIndex() const { return m_imageIndex; }
    InputType imageArgType() const { return m_imageArgType; }
    const AbstractSampler& sampler() const { return m_abstractSampler; }

    int tweenIndex() const { return m_tweenIndex; }

private:
    struct VGenInput {
        explicit VGenInput(float c): type(kConstant), dimension(1), vgenOutputIndex(0) { value.constant1 = c; }
        explicit VGenInput(glm::vec2 c): type(kConstant), dimension(2), vgenOutputIndex(0) { value.constant2 = c; }
        explicit VGenInput(glm::vec3 c): type(kConstant), dimension(3), vgenOutputIndex(0) { value.constant3 = c; }
        explicit VGenInput(glm::vec4 c): type(kConstant), dimension(4), vgenOutputIndex(0) { value.constant4 = c; }
        explicit VGenInput(size_t index, size_t out, size_t dim): type(kVGen), dimension(dim), vgenOutputIndex(out) {
            value.vgenIndex = index;
        }
        explicit VGenInput(size_t paramIndex): type(kParameter), dimension(1), vgenOutputIndex(0) {
            value.parameterIndex = paramIndex;
        }

        InputType type;
        size_t dimension;
        union Value {
            float constant1;
            size_t parameterIndex;
            glm::vec2 constant2;
            glm::vec3 constant3;
            glm::vec4 constant4;
            size_t vgenIndex;
        };
        Value value;
        size_t vgenOutputIndex;
    };

    std::shared_ptr<const AbstractVGen> m_abstractVGen;
    AbstractVGen::Rates m_rate;
    size_t m_imageIndex;
    InputType m_imageArgType;
    AbstractSampler m_abstractSampler;
    int m_tweenIndex;
    std::vector<VGenInput> m_inputs;
    std::vector<size_t> m_outputDimensions;
};

} // namespace base

} // namespace scin

#endif // SRC_BASE_VGEN_HPP_
