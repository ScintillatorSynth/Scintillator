#ifndef SRC_SCINTH_HPP_
#define SRC_SCINTH_HPP_

#include "core/Types.hpp"

#include <memory>

namespace scin {

class ScinthDef;

namespace vk {
class Buffer;
class Canvas;
class CommandBuffer;
class CommandPool;
class Device;
class Pipeline;
class Uniform;
class UniformLayout;
}

/*! Represents a running, controllable instance of a ScinthDef within the context of a vk::Canvas.
 */
class Scinth {
public:
    Scinth(std::shared_ptr<vk::Device> device, int nodeID, std::shared_ptr<ScinthDef> scinthDef);
    ~Scinth();

    /*! Do any one-time setup on this Scinth, including creating the Uniform Buffer.
     *
     * \param startTime The point in time that this Scinth should treat as its start time.
     * \param uniformLayout Uniform descriptor set, can be nullptr if no Uniform needed.
     * \param numberOfImages Number of images in the Canvas. Each image will get its own Uniform buffer to allow for
     *        independent rendering of the images.
     * \return true if successful, false if not.
     */
    bool create(const TimePoint& startTime, vk::UniformLayout* uniformLayout, size_t numberOfImages);

    /*! Build the CommandBuffers to render this Scinth. Can be called multiple times to rebuild them as needed.
     */
    bool buildBuffers(vk::CommandPool* commandPool, vk::Canvas* canvas, vk::Buffer* vertexBuffer,
                      vk::Buffer* indexBuffer, vk::Pipeline* pipeline);

    /*! Prepare for the next frame to render.
     *
     * Update the Uniform buffer associated with the imageIndex, if present, and any other operations to prepare a
     * frame to render at the provided time.
     *
     * \param imageIndex which of the images to prepare to render.
     * \param frameTime the time the frame is being prepared for.
     * \return true if Scinth should continue running for this frame, false otherwise.
     */
    bool prepareFrame(size_t imageIndex, const TimePoint& frameTime);

    /*! Determines the paused or playing status of the Scinth.
     *
     * \param run If false, will pause the Scinth. If true, will play it. Note this only sets the flag for the Scinth.
     */
    void setRunning(bool run) { m_running = run; }

    std::shared_ptr<vk::CommandBuffer> frameCommands() { return m_commands; }
    int nodeID() const { return m_nodeID; }
    bool running() const { return m_running; }

private:
    std::shared_ptr<vk::Device> m_device;
    int m_nodeID;
    // Keep a reference to the ScinthDef, so that it does not get deleted until all referring Scinths have also been
    // deleted.
    std::shared_ptr<ScinthDef> m_scinthDef;
    std::shared_ptr<vk::Uniform> m_uniform;
    std::shared_ptr<vk::CommandBuffer> m_commands;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
    bool m_running;
};

} // namespace scin

#endif // SRC_SCINTH_HPP_
