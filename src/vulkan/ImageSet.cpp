#include "vulkan/ImageSet.hpp"

#include "vulkan/Device.hpp"
#include "vulkan/Swapchain.hpp"

namespace scin { namespace vk {

ImageSet::ImageSet(std::shared_ptr<Device> device):
    m_device(device),
    m_nonOwning(true),
    m_format(VK_FORMAT_UNDEFINED) {}

ImageSet::~ImageSet() { destroy(); }

uint32_t ImageSet::getFromSwapchain(Swapchain* swapchain, uint32_t imageCount) {
    m_nonOwning = true;
    m_format = swapchain->surfaceFormat().format;
    m_extent = swapchain->extent();
    // Retrieve images from swap chain. Note it may be possible that Vulkan has allocated more images than requested by
    // the create call, so we query first for the actual image count.
    uint32_t actualImageCount = imageCount;
    vkGetSwapchainImagesKHR(m_device->get(), swapchain->get(), &actualImageCount, nullptr);
    m_images.resize(actualImageCount);
    vkGetSwapchainImagesKHR(m_device->get(), swapchain->get(), &actualImageCount, m_images.data());
    return actualImageCount;
}

bool ImageSet::create(uint32_t width, uint32_t height) {
    m_nonOwning = false;

    return true;
}

void ImageSet::destroy() {
    if (!m_nonOwning) {
    }
}

} // namespace vk

} // namespace scin
