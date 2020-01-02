#ifndef SRC_SCINTH_HPP_
#define SRC_SCINTH_HPP_

#include <chrono>
#include <memory>
#include <string>

namespace scin {

class AbstractScinthDef;

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

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

/*! Represents a running, controllable instance of a ScinthDef within the context of a vk::Canvas.
 */
class Scinth {
public:
    Scinth(std::shared_ptr<vk::Device> device, const std::string& name,
           std::shared_ptr<const AbstractScinthDef> abstractScinthDef);
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

    /*! Update the Uniform, if any, and provide the CommandBuffers to render this Scinth.
     */
    std::shared_ptr<vk::CommandBuffer> buildFrame(size_t imageIndex, const TimePoint& frameTime);

private:
    std::shared_ptr<vk::Device> m_device;
    std::string m_name;
    std::shared_ptr<const AbstractScinthDef> m_abstractScinthDef;
    std::unique_ptr<vk::Uniform> m_uniform;
    std::shared_ptr<vk::CommandBuffer> m_commands;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
};

} // namespace scin

#endif // SRC_SCINTH_HPP_
