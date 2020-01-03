#include "vulkan/UniformLayout.hpp"

#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

UniformLayout::UniformLayout(std::shared_ptr<Device> device): m_device(device), m_layout(VK_NULL_HANDLE) {}

UniformLayout::~UniformLayout() { destroy(); }

bool UniformLayout::create() {
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(m_device->get(), &layoutInfo, nullptr, &m_layout) != VK_SUCCESS) {
        spdlog::error("error creating descriptor set layout.");
        return false;
    }

    return true;
}

void UniformLayout::destroy() {
    if (m_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device->get(), m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
}

} // namespace vk

} // namespace scin
