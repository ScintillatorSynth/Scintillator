#include "Uniform.hpp"

#include "vulkan/Buffer.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Swapchain.hpp"
#include "vulkan/UniformLayout.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Uniform::Uniform(std::shared_ptr<Device> device): m_device(device), m_pool(VK_NULL_HANDLE) {}

Uniform::~Uniform() {}

bool Uniform::createBuffers(UniformLayout* layout, size_t size, size_t numberOfImages) {
    for (auto i = 0; i < numberOfImages; ++i) {
        std::shared_ptr<HostBuffer> buffer(new HostBuffer(Buffer::Kind::kUniform, size, m_device));
        if (!buffer->create()) {
            spdlog::error("error creating uniform buffer");
            return false;
        }
        m_buffers.push_back(buffer);
    }

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(numberOfImages);

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(numberOfImages);
    if (vkCreateDescriptorPool(m_device->get(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        spdlog::error("error creating uniform descriptor pool.");
        return false;
    }

    std::vector<VkDescriptorSetLayout> layouts(numberOfImages, layout->get());
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = numberOfImages;
    allocInfo.pSetLayouts = layouts.data();

    m_sets.resize(numberOfImages);
    if (vkAllocateDescriptorSets(m_device->get(), &allocInfo, m_sets.data()) != VK_SUCCESS) {
        spdlog::error("error allocating descriptor sets.");
        return false;
    }

    for (auto i = 0; i < numberOfImages; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_buffers[i]->buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = size;

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
}

} // namespace vk
} // namespace scin
