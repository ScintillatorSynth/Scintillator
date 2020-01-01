#include "vulkan/Images.hpp"

#include "vulkan/Swapchain.hpp"

namespace scin { namespace vk {

Images::Images(std::shared_ptr<Device> device): m_device(device), m_format(VK_FORMAT_UNDEFINED) {}

Images::~Images() {
    // For the moment the only way we have images is we got them from Swapchain, in which case they should not be
    // destroyed with VkDestroyImage, but rather are destroyed with VkDestroySwapchainKHR. So this dtor is a no-op
    // for the moment.
}

uint32_t Images::getFromSwapchain(Swapchain* swapchain, uint32_t imageCount) {
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

} // namespace vk

} // namespace scin
