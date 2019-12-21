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
    VGenInstance(std::shared_ptr<VGen> vgen);
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

private:
    struct VGenInput {
        enum InputType { kConstant, kVGen };
        explicit VGenInput(float constantValue): type(kConstant), constant(constantValue), vgenIndex(-1) {}
        explicit VGenInput(int index): type(kVGen), constant(0.0f), vgenIndex(index) {}

        InputType type;
        float constant;
        int vgenIndex;
    };

    std::shared_ptr<VGen> m_vgen;
    std::vector<VGenInput> m_inputs;
};

} // namespace scin

#endif // SRC_VGEN_INSTANCE_HPP_
