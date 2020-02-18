#include "vulkan/Sampler.hpp"

#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Sampler::Sampler(std::shared_ptr<Device> device, std::shared_ptr<HostImage> image):
    m_device(device),
    m_image(image),
    m_imageView(VK_NULL_HANDLE),
    m_sampler(VK_NULL_HANDLE) {}

Sampler::~Sampler() { destroy(); }

// TODO: just make this the sampler, and the server makes these separately, giving them IDs. So RenderImage.fg or
// something identifies both a sampler and a texture.

bool Sampler::create() {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image->get();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_image->format();
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(m_device->get(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        spdlog::error("Sampler failed to create ImageView.");
        return false;
    }

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (m_device->supportsSamplerAnisotropy()) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;
    } else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1;
    }
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(m_device->get(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        spdlog::error("Sampler failed to create sampler.");
        return false;
    }

    return true;
}

void Sampler::destroy() {
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device->get(), m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device->get(), m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
}


} // namespace vk
} // namespace scin
