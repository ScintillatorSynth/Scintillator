#ifndef SRC_VULKAN_FRAMEBUFFER_HPP_
#define SRC_VULKAN_FRAMEBUFFER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>

namespace scin { namespace vk {

class Canvas;
class Device;
class ImageSet;

/*! A container for an offscreen render target that can also be sampled as a texture and blitted from.
 */
class Framebuffer {
public:
    Framebuffer(std::shared_ptr<Device> device);
    ~Framebuffer();

    bool create(int width, int height, size_t numberOfImages);
    void destroy();

    VkFormat format();
    VkImage image(size_t index);
    std::shared_ptr<Canvas> canvas() { return m_canvas; }

private:
    std::shared_ptr<Device> m_device;

    std::shared_ptr<ImageSet> m_images;
    std::shared_ptr<Canvas> m_canvas;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_FRAMEBUFFER_HPP_
