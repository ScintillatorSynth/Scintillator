#ifndef SRC_SCINTH_HPP_
#define SRC_SCINTH_HPP_

#include <memory>
#include <string>

namespace scin {

class Shape;

namespace vk {
class Buffer;
class Canvas;
class CommandPool;
class CommandBuffer;
class Pipeline;
class Uniform;
}

/*! Represents a running, controllable instance of a ScinthDef.
 */
class Scinth {
public:
    Scinth(const std::string& name);
    ~Scinth();

    bool build(vk::CommandPool* commandPool, vk::Canvas* canvas, vk::Buffer* vertexBuffer, vk::Buffer* indexBuffer,
               vk::Pipeline* pipeline, vk::Uniform* uniform, Shape* shape);

private:
    std::string m_name;
    std::shared_ptr<vk::CommandBuffer> m_commands;
};

} // namespace scin

#endif // SRC_SCINTH_HPP_
