#include "vulkan/Framebuffer.hpp"

#include "vulkan/Canvas.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/ImageSet.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Framebuffer::Framebuffer(std::shared_ptr<Device> device): m_images(new ImageSet(device)), m_canvas(new Canvas(device)) {}

Framebuffer::~Framebuffer() { destroy(); }

bool Framebuffer::create(int with, int height, size_t numberOfImages) {
    if (!m_images->createFramebuffer(width, height, numberOfImages)) {
        spdlog::error("framebuffer failed to create images.");
        return false;
    }
    if (!m_canvas->create(m_images)) {
        spdlog::error("framebuffer failed to create canvas.");
        return false;
    }

    return true;
}

void Framebuffer::destroy() {
    m_canvas->destroy();
    m_images->destroy();
}

} // namespace vk

} // namespace scin
