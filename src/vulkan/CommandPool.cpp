#include "vulkan/CommandPool.hpp"

#include "vulkan/CommandBuffer.hpp"
#include "vulkan/Device.hpp"

namespace scin { namespace vk {

CommandPool::CommandPool(std::shared_ptr<Device> device): m_device(device), m_commandPool(VK_NULL_HANDLE) {}

CommandPool::~CommandPool() { destroy(); }

bool CommandPool::create() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_device->graphicsFamilyIndex();
    poolInfo.flags = 0;
    return (vkCreateCommandPool(m_device->get(), &poolInfo, nullptr, &m_commandPool) == VK_SUCCESS);
}

void CommandPool::destroy() {
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device->get(), m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
}

std::shared_ptr<CommandBuffer> CommandPool::createBuffers(size_t count) {
    std::shared_ptr<CommandBuffer> commandBuffer(new CommandBuffer(m_device, this));
    if (!commandBuffer->create(count)) {
        return nullptr;
    }
    return commandBuffer;
}

} // namespace vk

} // namespace scin
