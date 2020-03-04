#include "vulkan/Image.hpp"

#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Image::Image(std::shared_ptr<Device> device): m_device(device), m_image(VK_NULL_HANDLE) {}

Image::~Image() {}

Image::Image(std::shared_ptr<Device> device, VkImage image, VkFormat format, VkExtent2D extent):
    m_device(device),
    m_image(image),
    m_format(format),
    m_extent(extent) {}

SwapchainImage::SwapchainImage(std::shared_ptr<Device> device, VkImage image, VkFormat format, VkExtent2D extent):
    Image(device, image, format, extent) {}

SwapchainImage::~SwapchainImage() {}

AllocatedImage::AllocatedImage(std::shared_ptr<Device> device):
    Image(device, VK_NULL_HANDLE, VK_FORMAT_R8G8B8A8_UNORM, {}),
    m_allocation(VK_NULL_HANDLE) {}

AllocatedImage::~AllocatedImage() { destroy(); }

void AllocatedImage::destroy() {
    if (m_allocation != VK_NULL_HANDLE) {
        vmaDestroyImage(m_device->allocator(), m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
}

DeviceImage::DeviceImage(std::shared_ptr<Device> device): AllocatedImage(device) {}

DeviceImage::~DeviceImage() {}

bool DeviceImage::create(uint32_t width, uint32_t height) { return createDeviceImage(width, height, false); }

bool DeviceImage::createDeviceImage(uint32_t width, uint32_t height, bool isFramebuffer) {
    m_extent.width = width;
    m_extent.height = height;

    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = m_format;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.arrayLayers = 1;
    createInfo.mipLevels = 1;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    // TODO: the slow accretion of different usage bits is not a good sign, and prevents Vulkan from optimizing
    // these images. Repair.
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    if (isFramebuffer) {
        createInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(m_device->allocator(), &createInfo, &allocInfo, &m_image, &m_allocation, &m_info)
        != VK_SUCCESS) {
        spdlog::error("DeviceImage failed to create image.");
        return false;
    }

    return true;
}

FramebufferImage::FramebufferImage(std::shared_ptr<Device> device): DeviceImage(device) {}

FramebufferImage::~FramebufferImage() {}

bool FramebufferImage::create(uint32_t width, uint32_t height) { return createDeviceImage(width, height, true); }

HostImage::HostImage(std::shared_ptr<Device> device): AllocatedImage(device) {}

HostImage::~HostImage() {}

bool HostImage::create(uint32_t width, uint32_t height) {
    m_extent.width = width;
    m_extent.height = height;

    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = m_format;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.arrayLayers = 1;
    createInfo.mipLevels = 1;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_LINEAR;
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (vmaCreateImage(m_device->allocator(), &createInfo, &allocInfo, &m_image, &m_allocation, &m_info)
        != VK_SUCCESS) {
        spdlog::error("HostImage failed to create image.");
        return false;
    }

    return true;
}

} // namespace vk

} // namespace scin
