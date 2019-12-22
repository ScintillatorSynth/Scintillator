#ifndef SRC_VGEN_INSTANCE_HPP_
#define SRC_VGEN_INSTANCE_HPP_

#include <memory>
#include <vector>

namespace scin {

class VGen;

/*! Represents a single node in the signal flow graph of a ScinthDef. Combines a VGen and inputs.
 */
class VGenInstance {
public:
    VGenInstance(std::shared_ptr<const VGen> vgen);
    ~VGenInstance();

    /*! Adds a constant-valued input.
     *
     * \param constantValue The value of the constant to supply to this input.
     */
    void addConstantInput(float constantValue);

    /*! Add output from another VGen as input.
     *
     * \param index The index of the VGen within the ScinthDesc that will provide input.
     */
    void addVGenInput(int index);

    /*! Check this instance data against the originating VGen.
     *
     * \return true if valid, false if not.
     */
    bool validate();

    /*! If the input at index is a constant, return true and store the constant value in outValue.
     *
     * \param index The index of the input in question.
     * \param outValue Will be set to the value of the constant on successful completion of the function.
     * \return true if the input at index is a constant, false otherwise.
     */
    bool getInputConstantValue(int index, float& outValue) const;

    /*! If the input at index is a VGen, return true and store the vgen index value in outIndex.
     *
     * \param index The index of the input in question.
     * \param outIndex Will be set to the value of the index of VGen input on successful completion of the function.
     * \return true if the input at index is a VGen, false otherwise.
     */
    bool getInputVGenIndex(int index, int& outIndex) const;

    int numberOfInputs() const { return m_inputs.size(); }
    std::shared_ptr<const VGen> vgen() const { return m_vgen; }

private:
    struct VGenInput {
        enum InputType { kConstant, kVGen };
        explicit VGenInput(float constantValue): type(kConstant) { value.constant = constantValue; }
        explicit VGenInput(int index): type(kVGen) { value.vgenIndex = index; }

        InputType type;
        union Value {
            float constant;
            int vgenIndex;
        };
        Value value;
    };

    std::shared_ptr<const VGen> m_vgen;
    std::vector<VGenInput> m_inputs;
};

} // namespace scin

#endif // SRC_VGEN_INSTANCE_HPP_
