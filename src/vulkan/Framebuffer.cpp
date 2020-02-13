#include "vulkan/Framebuffer.hpp"

#include "vulkan/Canvas.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Framebuffer::Framebuffer(std::shared_ptr<Device> device): m_canvas(new Canvas(device)) {}

Framebuffer::~Framebuffer() { destroy(); }

bool Framebuffer::create(int width, int height, size_t numberOfImages) {
    std::vector<VkImage> vulkanImages;
    for (auto i = 0; i < numberOfImages; ++i) {
        FramebufferImage image(m_device);
        if (!image.create(width, height)) {
            spdlog::error("framebuffer failed to create images.");
            return false;
        }
        vulkanImages.emplace_back(image.get());
        m_images.emplace_back(image);
    }

    if (!m_canvas->create(vulkanImages, width, height, m_images[0].format())) {
        spdlog::error("framebuffer failed to create canvas.");
        return false;
    }

    return true;
}

void Framebuffer::destroy() {
    m_canvas->destroy();
    m_images.clear();
}

VkFormat Framebuffer::format() { return m_images[0].format(); }
VkImage Framebuffer::image(size_t index) { return m_images[index].get(); }

} // namespace vk

} // namespace scin
