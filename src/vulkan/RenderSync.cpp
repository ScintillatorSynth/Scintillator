#include "vulkan/RenderSync.hpp"

#include "vulkan/Device.hpp"
#include "vulkan/Swapchain.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

RenderSync::RenderSync(std::shared_ptr<Device> device): m_device(device) {}

RenderSync::~RenderSync() { destroy(); }

bool RenderSync::create(size_t inFlightFrames, bool makeSemaphores) {
    if (makeSemaphores) {
        m_imageAvailable.resize(inFlightFrames);
        m_renderFinished.resize(inFlightFrames);
    }
    m_frameRendering.resize(inFlightFrames);

    for (auto i = 0; i < inFlightFrames; ++i) {
        if (makeSemaphores) {
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            if (vkCreateSemaphore(m_device->get(), &semaphoreInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS) {
                spdlog::error("RenderSync failed to create image available semaphore.");
                return false;
            }
            if (vkCreateSemaphore(m_device->get(), &semaphoreInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS) {
                spdlog::error("RenderSync failed to create render finished semaphore.");
                return false;
            }
        }

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (vkCreateFence(m_device->get(), &fenceInfo, nullptr, &m_frameRendering[i]) != VK_SUCCESS) {
            spdlog::error("RenderSync failed to create render fence.");
            return false;
        }
    }
    return true;
}

void RenderSync::destroy() {
    for (auto i = 0; i < m_imageAvailable.size(); ++i) {
        vkDestroySemaphore(m_device->get(), m_imageAvailable[i], nullptr);
    }
    m_imageAvailable.clear();

    for (auto i = 0; i < m_renderFinished.size(); ++i) {
        vkDestroySemaphore(m_device->get(), m_renderFinished[i], nullptr);
    }
    m_renderFinished.clear();

    for (auto i = 0; i < m_frameRendering.size(); ++i) {
        vkDestroyFence(m_device->get(), m_frameRendering[i], nullptr);
    }
    m_frameRendering.clear();
}

void RenderSync::waitForFrame(size_t index) {
    vkWaitForFences(m_device->get(), 1, &m_frameRendering[index], VK_TRUE, std::numeric_limits<uint64_t>::max());
}

uint32_t RenderSync::acquireNextImage(size_t index, Swapchain* swapchain) {
    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(m_device->get(), swapchain->get(), std::numeric_limits<uint64_t>::max(),
                          m_imageAvailable[index], VK_NULL_HANDLE, &imageIndex);
    return imageIndex;
}

void RenderSync::resetFrame(size_t index) { vkResetFences(m_device->get(), 1, &m_frameRendering[index]); }

} // namespace vk

} // namespace scin
