#include "vulkan/Sampler.hpp"

#include "base/AbstractSampler.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"

#include "spdlog/spdlog.h"

namespace {

VkFilter filterForAbstract(scin::base::AbstractSampler::FilterMode filterMode) {
    switch (filterMode) {
    case scin::base::AbstractSampler::FilterMode::kLinear:
        return VK_FILTER_LINEAR;

    case scin::base::AbstractSampler::FilterMode::kNearest:
        return VK_FILTER_NEAREST;
    }
}

VkSamplerAddressMode addressModeForAbstract(scin::base::AbstractSampler::AddressMode addressMode) {
    switch (addressMode) {
    case scin::base::AbstractSampler::AddressMode::kClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

    case scin::base::AbstractSampler::AddressMode::kClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    case scin::base::AbstractSampler::AddressMode::kRepeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;

    case scin::base::AbstractSampler::AddressMode::kMirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
}

VkBorderColor borderColorForAbstract(scin::base::AbstractSampler::ClampBorderColor borderColor) {
    switch (borderColor) {
    case scin::base::AbstractSampler::ClampBorderColor::kTransparentBlack:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

    case scin::base::AbstractSampler::ClampBorderColor::kBlack:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

    case scin::base::AbstractSampler::ClampBorderColor::kWhite:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    }
}

} // namespace

namespace scin { namespace vk {

Sampler::Sampler(std::shared_ptr<Device> device, const base::AbstractSampler& abstractSampler):
    m_device(device),
    m_abstractSampler(abstractSampler),
    m_sampler(VK_NULL_HANDLE) {}

Sampler::~Sampler() { destroy(); }

bool Sampler::create() {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.minFilter = filterForAbstract(m_abstractSampler.minFilterMode());
    samplerInfo.magFilter = filterForAbstract(m_abstractSampler.magFilterMode());

    samplerInfo.addressModeU = addressModeForAbstract(m_abstractSampler.addressModeU());
    samplerInfo.addressModeV = addressModeForAbstract(m_abstractSampler.addressModeV());
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

    if (m_device->supportsSamplerAnisotropy() && m_abstractSampler.isAnisotropicFilteringEnabled()) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;
    } else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1;
    }

    samplerInfo.borderColor = borderColorForAbstract(m_abstractSampler.clampBorderColor());
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
}


} // namespace vk
} // namespace scin
