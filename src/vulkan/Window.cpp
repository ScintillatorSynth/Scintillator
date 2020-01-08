#include "vulkan/Window.hpp"

#include "Compositor.hpp"
#include "vulkan/Canvas.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/ImageSet.hpp"
#include "vulkan/Instance.hpp"
#include "vulkan/Swapchain.hpp"

#include "spdlog/spdlog.h"

#include <chrono>
#include <limits>

const size_t kMaxFramesInFlight = 2;

namespace scin { namespace vk {

Window::Window(std::shared_ptr<Instance> instance):
    m_instance(instance),
    m_width(-1),
    m_height(-1),
    m_window(nullptr),
    m_surface(VK_NULL_HANDLE),
    m_stop(false) {}

Window::~Window() {}

bool Window::create(int width, int height, bool keepOnTop) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, keepOnTop ? GLFW_TRUE : GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, "ScintillatorSynth", nullptr, nullptr);
    if (glfwCreateWindowSurface(m_instance->get(), m_window, nullptr, &m_surface) != VK_SUCCESS) {
        spdlog::error("failed to create surface");
        return false;
    }

    m_width = width;
    m_height = height;

    return true;
}

bool Window::createSwapchain(std::shared_ptr<Device> device) {
    m_device = device;
    m_swapchain.reset(new Swapchain(m_device));
    if (!m_swapchain->create(this)) {
        spdlog::error("Window failed to create swapchain.");
        return false;
    }

    m_readbackImages.reset(new ImageSet(m_device));
    if (!m_readbackImages->createHostTransferTarget(m_width, m_height, m_swapchain->numberOfImages())) {
        spdlog::error("Window failed to create readback images.");
        return false;
    }

    return true;
}

bool Window::createSyncObjects() {
    m_imageAvailableSemaphores.resize(kMaxFramesInFlight);
    m_renderFinishedSemaphores.resize(kMaxFramesInFlight);
    m_inFlightFences.resize(kMaxFramesInFlight);
    m_commandBuffers.resize(kMaxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (vkCreateSemaphore(m_device->get(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS) {
            return false;
        }
        if (vkCreateSemaphore(m_device->get(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
            return false;
        }
        if (vkCreateFence(m_device->get(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

void Window::run(std::shared_ptr<Compositor> compositor) {
    size_t currentFrame = 0;

    while (!m_stop && !glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // Wait for the number of in-flight images to come down to max-1 by waiting on the inflight fence for this
        // frame.
        vkWaitForFences(m_device->get(), 1, &m_inFlightFences[currentFrame], VK_TRUE,
                        std::numeric_limits<uint64_t>::max());

        // Ask VulkanKHR which is the next available image in the swapchain, wait indefinitely for it to become
        // available.
        uint32_t imageIndex;
        vkAcquireNextImageKHR(m_device->get(), m_swapchain->get(), std::numeric_limits<uint64_t>::max(),
                              m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        m_commandBuffers[currentFrame] =
            compositor->prepareFrame(imageIndex, std::chrono::high_resolution_clock::now());

        VkSemaphore imageAvailable[] = { m_imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore renderFinished[] = { m_renderFinishedSemaphores[currentFrame] };
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = imageAvailable;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer = m_commandBuffers[currentFrame]->buffer(imageIndex);
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = renderFinished;

        // Reset the inflight fence to indicate that the frame is about to become in-flight again.
        vkResetFences(m_device->get(), 1, &m_inFlightFences[currentFrame]);

        // Submits the command buffer to the queue. Won't start the buffer until the image is marked as available by
        // the imageAvailable semaphore, and when the render is finished it will signal the renderFinished semaphore
        // on the device, and also clear the inflight fence on the host.
        if (vkQueueSubmit(m_device->graphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS) {
            break;
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = renderFinished;
        VkSwapchainKHR swapchains[] = { m_swapchain->get() };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;
        // Queue the frame to be presented as soon as the renderFinished semaphore is signaled by the end of the
        // command buffer execution.
        vkQueuePresentKHR(m_device->presentQueue(), &presentInfo);

        // *** At this point in time the CPU could wait on the inflight fence for the frame just submitted (before
        // advancing the frame counter below), and if there was a copy operation requested this would be the optimal
        // time to do queue it as the image will not be needed until its turn in the swapchain again, and presumably
        // the CPU is not needed until it is time to queue another command buffer for the next inflight.

        currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
    }

    vkDeviceWaitIdle(m_device->get());
    m_commandBuffers.clear();
}

void Window::destroySyncObjects() {
    for (auto semaphore : m_imageAvailableSemaphores) {
        vkDestroySemaphore(m_device->get(), semaphore, nullptr);
    }
    m_imageAvailableSemaphores.clear();

    for (auto semaphore : m_renderFinishedSemaphores) {
        vkDestroySemaphore(m_device->get(), semaphore, nullptr);
    }
    m_renderFinishedSemaphores.clear();

    for (auto fence : m_inFlightFences) {
        vkDestroyFence(m_device->get(), fence, nullptr);
    }
    m_inFlightFences.clear();
}

void Window::destroySwapchain() { m_swapchain->destroy(); }

void Window::destroy() {
    vkDestroySurfaceKHR(m_instance->get(), m_surface, nullptr);
    glfwDestroyWindow(m_window);
}

std::shared_ptr<Canvas> Window::canvas() { return m_swapchain->canvas(); }

} // namespace vk

} // namespace scin
