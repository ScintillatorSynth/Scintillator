#ifndef SRC_COMP_SCINTH_HPP_
#define SRC_COMP_SCINTH_HPP_

#include <memory>

namespace scin {

namespace vk {
class Buffer;
class CommandBuffer;
class CommandPool;
class Device;
}

namespace comp {

class ScinthDef;

/*! Represents a running, controllable instance of a ScinthDef.
 */
class Scinth {
public:
    Scinth(std::shared_ptr<vk::Device> device, int nodeID, std::shared_ptr<ScinthDef> scinthDef);
    ~Scinth();

    /*! Do any one-time setup on this Scinth, including creating the Uniform Buffer.
     *
     * \return true if successful, false if not.
     */
    bool create();

    /*! Prepare for the next frame to render.
     *
     * Update the Uniform buffer associated with the imageIndex, if present, and any other operations to prepare a
     * frame to render at the provided time.
     *
     * \param imageIndex which of the images to prepare to render.
     * \param frameTime the time the frame is being prepared for.
     * \return true if Scinth should continue running for this frame, false otherwise.
     */
    bool prepareFrame(size_t imageIndex, double frameTime);

    /*! Determines the paused or playing status of the Scinth.
     *
     * \param run If false, will pause the Scinth. If true, will play it. Note this only sets the flag for the Scinth.
     */
    void setRunning(bool run) { m_running = run; }

    /*! Sets a parameter value by name. Will cause a command buffer rebuild on next call to prepareFrame().
     *
     * \param name The name of the parameter to set.
     * \param value The value of the parameter.
     */
    void setParameterByName(const std::string& name, float value);

    /*! Sets a parameter value by index. Will cause a command buffer rebuild on next call to prepareFrame().
     *
     * \param index The index of the parameter to set.
     * \param value The new value of the parameter.
     */
    void setParameterByIndex(int index, float value);

    std::shared_ptr<vk::CommandBuffer> frameCommands() { return m_commands; }
    int nodeID() const { return m_nodeID; }
    bool running() const { return m_running; }

private:
    bool buildDescriptors();
    bool rebuildBuffers();

    std::shared_ptr<vk::Device> m_device;
    int m_nodeID;
    bool m_cueued;
    double m_startTime;

    // Keep a reference to the ScinthDef, so that it does not get deleted until all referring Scinths have also been
    // deleted.
    std::shared_ptr<ScinthDef> m_scinthDef;
    std::shared_ptr<vk::CommandBuffer> m_commands;
    bool m_running;
    std::unique_ptr<float[]> m_parameterValues;
    size_t m_numberOfParameters;
    bool m_commandBuffersDirty;
};

} // namespace comp

} // namespace scin

#endif // SRC_COMP_SCINTH_HPP_
