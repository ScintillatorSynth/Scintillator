#ifndef SRC_CORE_VGEN_HPP_
#define SRC_CORE_VGEN_HPP_

#include <memory>
#include <vector>

#include "glm/glm.hpp"

namespace scin {

class AbstractVGen;

/*! Represents a single node in the signal flow graph of a ScinthDef. Combines an AbstractVGen and inputs.
 */
class VGen {
public:
    VGen(std::shared_ptr<const AbstractVGen> abstractVGen);
    ~VGen();

    /*! Adds a single-dimensional constant-valued input.
     *
     * \param constantValue The value of the constant to supply to this input.
     */
    void addConstantInput(float constantValue);
    void addConstantInput(glm::vec2 constantValue);
    void addConstantInput(glm::vec3 constantValue);
    void addConstantInput(glm::vec4 constantValue);

    /*! Add output from another VGen as input.
     *
     * \note  These values are not validated as they require knowledge outside of this VGen to validate.
     * \param vgenIndex The index of the VGen within the ScinthDef that will provide input.
     * \param outputIndex The index of the output from the VGen use as input.
     * \param dimension The dimension of the input.
     */
    void addVGenInput(int vgenIndex, int outputIndex, int dimension);

    /*! Add an output to this VGen with supplied dimension.
     *
     * \param dimension The dimension of the output.
     */
    void addOutput(int dimension);

    /*! Check this instance data against the originating AbstractVGen.
     *
     * \return true if valid, false if not.
     */
    bool validate() const;

    /*! If the input at index is a constant, return true and store the constant value in outValue.
     *
     * \param index The index of the input in question.
     * \param outValue Will be set to the value of the constant on successful completion of the function.
     * \return true if the input at index is a constant, false otherwise.
     */
    bool getInputConstantValue(int index, float& outValue) const;
    bool getInputConstantValue(int index, glm::vec2& outValue) const;
    bool getInputConstantValue(int index, glm::vec3& outValue) const;
    bool getInputConstantValue(int index, glm::vec4& outValue) const;

    /*! If the input at index is a VGen, return true and store the vgen index value in outIndex.
     *
     * \param index The index of the input in question.
     * \param outIndex Will be set to the value of the index of VGen's output on successful completion of the function.
     * \param outOutput Will be set to the value of the index of the output on the selected VGen.
     * \return true if the input at index is a VGen, false otherwise.
     */
    bool getInputVGenIndex(int index, int& outIndex, int& outOutput) const;

    int numberOfInputs() const { return m_inputs.size(); }

    int numberOfOutputs() const { return m_outputDimensions.size(); }
    int outputDimension(int index) const { return m_outputDimensions[index]; }

    std::shared_ptr<const AbstractVGen> abstractVGen() const { return m_abstractVGen; }

private:
    struct VGenInput {
        enum InputType { kConstant, kVGen };
        explicit VGenInput(float c): type(kConstant), dimension(1) { value.constant1 = c; }
        explicit VGenInput(glm::vec2 c): type(kConstant), dimension(2) { value.constant2 = c; }
        explicit VGenInput(glm::vec3 c): type(kConstant), dimension(3) { value.constant3 = c; }
        explicit VGenInput(glm::vec4 c): type(kConstant), dimension(4) { value.constant4 = c; }
        explicit VGenInput(int index, int out, int dim): type(kVGen), dimension(dim) {
            value.vgen = glm::ivec2(index, out);
        }

        InputType type;
        int dimension;
        union Value {
            float constant1;
            glm::vec2 constant2;
            glm::vec3 constant3;
            glm::vec4 constant4;
            glm::ivec2 vgen;
        };
        Value value;
    };

    std::shared_ptr<const AbstractVGen> m_abstractVGen;
    std::vector<VGenInput> m_inputs;
    std::vector<int> m_outputDimensions;
};

} // namespace scin

#endif // SRC_CORE_VGEN_HPP_
