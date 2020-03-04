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
    m_hasCommands(false),
    m_quit(false) {}

StageManager::~StageManager() { destroy(); }

bool StageManager::create(size_t numberOfImages) {
    if (!m_commandPool->create()) {
        spdlog::error("StageManager failed to create command pool.");
        return false;
    }

    for (auto i = 0; i < numberOfImages; ++i) {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence = VK_NULL_HANDLE;
        if (vkCreateFence(m_device->get(), &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
            spdlog::error("StageManager failed to create fence.");
            return false;
        }
        m_fences.emplace_back(fence);
    }

    m_callbackThread = std::thread(&StageManager::callbackThreadMain, this);
    return true;
}

void StageManager::destroy() {
    if (!m_quit) {
        m_quit = true;
        m_waitCondition.notify_all();
        m_callbackThread.join();
    }

    m_commandPool = nullptr;
    m_pendingWait = Wait();
    m_waits.clear();

    for (auto fence : m_fences) {
        vkDestroyFence(m_device->get(), fence, nullptr);
    }
    m_fences.clear();
}

bool StageManager::stageImage(std::shared_ptr<vk::HostBuffer> hostBuffer, std::shared_ptr<vk::DeviceImage> deviceImage,
                              std::function<void()> completion) {
    std::lock_guard<std::mutex> lock(m_commandMutex);
    if (!m_hasCommands) {
        m_pendingWait.commands.reset(new vk::CommandBuffer(m_device, m_commandPool));
        if (!m_pendingWait.commands->create(1, true)) {
            spdlog::error("StageManager failed to create command buffer for transfer.");
            return false;
        }
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(m_pendingWait.commands->buffer(0), &beginInfo) != VK_SUCCESS) {
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
    vkCmdPipelineBarrier(m_pendingWait.commands->buffer(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
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
    vkCmdCopyBufferToImage(m_pendingWait.commands->buffer(0), hostBuffer->buffer(), deviceImage->get(),
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
    vkCmdPipelineBarrier(m_pendingWait.commands->buffer(0), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &samplerBarrier);

    m_pendingWait.callbacks.push_back(completion);
    m_pendingWait.hostBuffers.push_back(hostBuffer);
    m_pendingWait.deviceImages.push_back(deviceImage);
    return true;
}

bool StageManager::submitTransferCommands(VkQueue queue) {
    if (!m_hasCommands) {
        return true;
    }

    Wait wait;
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        wait = m_pendingWait;
        m_pendingWait = Wait();
        m_hasCommands = false;
        vkEndCommandBuffer(wait.commands->buffer(0));
        m_pendingWait.fenceIndex = (wait.fenceIndex + 1) % m_fences.size();
    }

    vkResetFences(m_device->get(), 1, &m_fences[wait.fenceIndex]);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffers[] = { wait.commands->buffer(0) };
    submitInfo.pCommandBuffers = commandBuffers;

    if (vkQueueSubmit(m_device->graphicsQueue(), 1, &submitInfo, m_fences[wait.fenceIndex]) != VK_SUCCESS) {
        spdlog::error("StageManager failed to submit transfer command buffer to graphics queue.");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_waitMutex);
        m_waits.emplace_back(wait);
    }
    m_waitCondition.notify_one();

    return true;
}

void StageManager::callbackThreadMain() {
    while (!m_quit) {
        Wait wait;
        bool hasWait = false;
        {
            std::unique_lock<std::mutex> lock(m_waitMutex);
            m_waitCondition.wait(lock, [this] { return m_quit || m_waits.size(); });
            if (m_quit) {
                spdlog::info("StageManager work thread got quit wakeup, exiting.");
                break;
            }

            if (m_waits.size()) {
                wait = m_waits.front();
                hasWait = true;
            }
        }

        while (!m_quit && hasWait) {
            vkWaitForFences(m_device->get(), 1, &m_fences[wait.fenceIndex], VK_TRUE,
                            std::numeric_limits<uint64_t>::max());
            if (m_quit) {
                break;
            }
            for (auto callback : wait.callbacks) {
                callback();
            }

            {
                std::lock_guard<std::mutex> lock(m_waitMutex);
                if (m_waits.size()) {
                    wait = m_waits.front();
                    m_waits.pop_front();
                } else {
                    hasWait = false;
                }
            }
        }
    }

    spdlog::info("StageManager work thread terminated.");
}

} // namespace comp
} // namespace scin
