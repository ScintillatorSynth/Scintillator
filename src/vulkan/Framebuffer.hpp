#ifndef SRC_VULKAN_FRAMEBUFFER_HPP_
#define SRC_VULKAN_FRAMEBUFFER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin { namespace vk {

class Canvas;
class Device;
class FramebufferImage;

/*! A container for an offscreen render target that can also be sampled as a texture and blitted from.
 *
 * TODO: Kinda weird that *Canvas* is actually the keeper of the VkFramebuffer obejcts. Consider merging this class
 * either down or up, like pushing this into either Canvas or owners of Framebuffer (Offscreen? Any other takers?)
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

    std::vector<FramebufferImage> m_images;
    std::shared_ptr<Canvas> m_canvas;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_FRAMEBUFFER_HPP_
