#include "vulkan/CommandBuffer.hpp"

#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

CommandBuffer::CommandBuffer(const std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool):
    m_device(device),
    m_commandPool(commandPool) {}

CommandBuffer::~CommandBuffer() { destroy(); }

bool CommandBuffer::create(size_t count, bool isPrimary) {
    m_commandBuffers.resize(count);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool->get();
    if (isPrimary) {
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    } else {
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    }
    allocInfo.commandBufferCount = static_cast<uint32_t>(count);

    if (vkAllocateCommandBuffers(m_device->get(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        spdlog::error("error allocating command buffers.");
        return false;
    }

    return true;
}

void CommandBuffer::destroy() {
    spdlog::debug("CommandBuffer destructor");
    m_scinth = nullptr;
    m_secondaryCommands.clear();

    if (m_commandBuffers.size()) {
        vkFreeCommandBuffers(m_device->get(), m_commandPool->get(), static_cast<uint32_t>(m_commandBuffers.size()),
            m_commandBuffers.data());
        m_commandBuffers.clear();
    }
}

void CommandBuffer::associateScinth(std::shared_ptr<comp::Scinth> scinth) { m_scinth = scinth; }

void CommandBuffer::associateSecondaryCommands(const std::vector<std::shared_ptr<CommandBuffer>>& commands) {
    m_secondaryCommands.insert(m_secondaryCommands.begin(), commands.begin(), commands.end());
}

} // namespace vk

} // namespace scin
