#include "vulkan/ImageSet.hpp"

#include "vulkan/Device.hpp"
#include "vulkan/Swapchain.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

ImageSet::ImageSet(std::shared_ptr<Device> device): m_device(device), m_format(VK_FORMAT_UNDEFINED) {}

ImageSet::~ImageSet() { destroy(); }

uint32_t ImageSet::getFromSwapchain(Swapchain* swapchain, uint32_t imageCount) {
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

bool ImageSet::createHostTransferTarget(uint32_t width, uint32_t height, size_t numberOfImages) {
    return true;
    m_images.resize(numberOfImages);
    m_allocations.resize(numberOfImages);
    m_extent.width = width;
    m_extent.height = height;

    for (auto i = 0; i < numberOfImages; ++i) {
        VkImageCreateInfo createInfo = {};
        createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        createInfo.extent.width = width;
        createInfo.extent.height = height;
        createInfo.arrayLayers = 1;
        createInfo.mipLevels = 1;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.tiling = VK_IMAGE_TILING_LINEAR;
        createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        VkImage image;
        VmaAllocation allocation;
        if (vmaCreateImage(m_device->allocator(), &createInfo, &allocInfo, &image, &allocation, nullptr)
            != VK_SUCCESS) {
            spdlog::error("failed creating host transfer target images");
            return false;
        }

        m_images.emplace_back(image);
        m_allocations.emplace_back(allocation);
    }

    return true;
}

void ImageSet::destroy() {
    // Any images allocated by this ImageSet will have allocations associated, so we reclaim any of those here.
    for (auto i = 0; i <= m_allocations.size(); ++i) {
        vmaDestroyImage(m_device->allocator(), m_images[i], m_allocations[i]);
    }
    m_images.clear();
    m_allocations.clear();
}

} // namespace vk

} // namespace scin
