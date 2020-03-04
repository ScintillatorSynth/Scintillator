#include "comp/StageManager.hpp"

#include "vulkan/Buffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"

#include "spdlog/spdlog.h"

#include <limits>

namespace scin { namespace comp {

StageManager::StageManager(std::shared_ptr<vk::Device> device):
    m_device(device),
    m_commandPool(new vk::CommandPool(device)),
    m_quit(false) {}

StageManager::~StageManager() { destroy(); }

bool StageManager::create() {
    if (!m_commandPool->create()) {
        spdlog::error("StageManager failed to create command pool.");
        return false;
    }

    m_callbackThread = std::thread(&StageManager::callbackThreadMain, this);
    return true;
}

void StageManager::destroy() {
    if (!m_quit) {
        m_quit = true;
        m_waitActiveCondition.notify_all();
        m_callbackThread.join();
    }

    m_transferCommands.reset();
    m_commandPool = nullptr;
}

bool StageManager::stageImage(std::shared_ptr<vk::HostBuffer> hostBuffer, std::shared_ptr<vk::DeviceImage> deviceImage,
                              std::function<void()> completion) {
    std::lock_guard<std::mutex> lock(m_commandMutex);
    if (!m_transferCommands) {
        m_transferCommands.reset(new vk::CommandBuffer(m_device, m_commandPool));
        if (!m_transferCommands->create(1, true)) {
            spdlog::error("StageManager failed to create command buffer for transfer.");
            return false;
        }
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(m_transferCommands->buffer(0), &beginInfo) != VK_SUCCESS) {
            spdlog::error("StageManager failed to begin command buffer.");
            return false;
        }
        m_hasCommands = true;
    }

    VkImageMemoryBarrier transferBarrier = {};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transferBarrier.image = deviceImage->get();
    transferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    transferBarrier.subresourceRange.baseMipLevel = 0;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = 0;
    transferBarrier.subresourceRange.layerCount = 1;
    transferBarrier.srcAccessMask = 0;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(m_transferCommands->buffer(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferBarrier);

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = { 0, 0, 0 };
    copyRegion.imageExtent = { deviceImage->width(), deviceImage->height(), 1 };
    vkCmdCopyBufferToImage(m_transferCommands->buffer(0), hostBuffer->buffer(), deviceImage->get(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    VkImageMemoryBarrier samplerBarrier = {};
    samplerBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    samplerBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    samplerBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    samplerBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    samplerBarrier.image = deviceImage->get();
    samplerBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    samplerBarrier.subresourceRange.baseMipLevel = 0;
    samplerBarrier.subresourceRange.levelCount = 1;
    samplerBarrier.subresourceRange.baseArrayLayer = 0;
    samplerBarrier.subresourceRange.layerCount = 1;
    samplerBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    samplerBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(m_transferCommands->buffer(0), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &samplerBarrier);

    m_callbacks.push_back(completion);
    return true;
}

std::shared_ptr<vk::CommandBuffer> StageManager::getTransferCommands(VkFence renderFence) {
    std::shared_ptr<vk::CommandBuffer> commands;
    if (!m_hasCommands) {
        return commands;
    }

    std::vector<std::function<void()>> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        commands = m_transferCommands;
        if (commands) {
            vkEndCommandBuffer(commands->buffer(0));
        }
        m_transferCommands = nullptr;
        m_hasCommands = false;
        callbacks.assign(m_callbacks.begin(), m_callbacks.end());
        m_callbacks.clear();
    }

    // If we ended up with callbacks to process on this command buffer enqueue them and kick off the waiting thread.
    if (callbacks.size()) {
        {
            std::lock_guard<std::mutex> lock(m_waitPairsMutex);
            m_waitPairs.emplace_back(std::make_pair(renderFence, callbacks));
        }
        m_waitActiveCondition.notify_one();
    }

    return commands;
}

void StageManager::callbackThreadMain() {
    while (!m_quit) {
        VkFence fence = VK_NULL_HANDLE;
        std::vector<std::function<void()>> callbacks;

        {
            std::unique_lock<std::mutex> lock(m_waitPairsMutex);
            m_waitActiveCondition.wait(lock, [this] { return m_quit || m_waitPairs.size(); });
            if (m_quit) {
                spdlog::info("StageManager work thread got quit wakeup, exiting.");
                break;
            }

            if (m_waitPairs.size()) {
                fence = m_waitPairs.front().first;
                callbacks = m_waitPairs.front().second;
                m_waitPairs.pop_front();
            }
        }

        while (!m_quit && fence != VK_NULL_HANDLE) {
            vkWaitForFences(m_device->get(), 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
            if (m_quit) {
                break;
            }
            for (auto callback : callbacks) {
                callback();
            }
            {
                std::lock_guard<std::mutex> lock(m_waitPairsMutex);
                if (m_waitPairs.size()) {
                    fence = m_waitPairs.front().first;
                    callbacks = m_waitPairs.front().second;
                    m_waitPairs.pop_front();
                } else {
                    fence = VK_NULL_HANDLE;
                }
            }
        }
    }

    spdlog::info("StageManager work thread terminated.");
}

} // namespace comp
} // namespace scin
