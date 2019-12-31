#include "vulkan/CommandBuffer.hpp"

#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

CommandBuffer::CommandBuffer(const std::shared_ptr<Device> device, CommandPool* commandPool): m_device(device),
    m_commandPool(commandPool) {}

CommandBuffer::~CommandBuffer() { destroy(); }

bool CommandBuffer::create(size_t count) {
    m_commandBuffers.resize(count);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool->get();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    if (vkAllocateCommandBuffers(m_device->get(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        spdlog::error("error allocating command buffers.");
        return false;
    }

    return true;
}

void CommandBuffer::destroy() {
    if (m_commandBuffers.size()) {
        vkFreeCommandBuffers(m_device->get(), m_commandPool->get(), m_commandBuffers.size(), m_commandBuffers.data());
        m_commandBuffers.clear();
    }
}

} // namespace vk

} // namespace scin