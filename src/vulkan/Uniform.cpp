#include "Uniform.hpp"

#include "vulkan/Buffer.hpp"
#include "vulkan/device.h"
#include "vulkan/swapchain.h"

#include "spdlog/spdlog.h"

namespace scin {
namespace vk {

Uniform::Uniform(std::shared_ptr<Device> device, size_t size) :
    m_device(device),
    m_size(size),
    m_layout(VK_NULL_HANDLE),
    m_pool(VK_NULL_HANDLE) {
}

Uniform::~Uniform() {
}

bool Uniform::createLayout() {
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

bool Uniform::createBuffers(Swapchain* swapchain) {
    for (auto i = 0; i < swapchain->image_count(); ++i) {
        std::shared_ptr<Buffer> buffer(new Buffer(Buffer::Kind::kUniform, m_device));
        if (!buffer->Create(m_size)) {
            spdlog::error("error creating uniform buffer");
            return false;
        }
        m_buffers.push_back(buffer);
    }

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(swapchain->image_count());

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(swapchain->image_count());
    if (vkCreateDescriptorPool(m_device->get(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        spdlog::error("error creating uniform descriptor pool.");
        return false;
    }

    std::vector<VkDescriptorSetLayout> layouts(swapchain->image_count(), m_layout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = swapchain->image_count();
    allocInfo.pSetLayouts = layouts.data();

    m_sets.resize(swapchain->image_count());
    if (vkAllocateDescriptorSets(m_device->get(), &allocInfo, m_sets.data()) != VK_SUCCESS) {
        spdlog::error("error allocating descriptor sets.");
        return false;
    }

    for (auto i = 0; i < swapchain->image_count(); ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_buffers[i]->buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = m_size;

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_sets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;
        write.pImageInfo = nullptr;
        write.pTexelBufferView = nullptr;
        vkUpdateDescriptorSets(m_device->get(), 1, &write, 0, nullptr);
    }

    return true;
}

void Uniform::destroy() {
    m_buffers.clear();
    vkDestroyDescriptorPool(m_device->get(), m_pool, nullptr);
    vkDestroyDescriptorSetLayout(m_device->get(), m_layout, nullptr);
}

}  // namespace vk
}  // namespace scin

