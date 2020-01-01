#include "vulkan/Window.hpp"

#include "Compositor.hpp"
#include "vulkan/Canvas.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
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

bool Window::create(int width, int height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
    return m_swapchain->create(this);
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

        vkWaitForFences(m_device->get(), 1, &m_inFlightFences[currentFrame], VK_TRUE,
                        std::numeric_limits<uint64_t>::max());

        uint32_t imageIndex;
        vkAcquireNextImageKHR(m_device->get(), m_swapchain->get(), std::numeric_limits<uint64_t>::max(),
                              m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        m_commandBuffers[currentFrame] = compositor->buildFrame(imageIndex);
        std::vector<VkCommandBuffer> commandBuffers;
        for (auto command : m_commandBuffers[currentFrame]) {
            commandBuffers.push_back(command->buffer(imageIndex));
        }
        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] };

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = commandBuffers.size();
        submitInfo.pCommandBuffers = commandBuffers.data();
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(m_device->get(), 1, &m_inFlightFences[currentFrame]);

        if (vkQueueSubmit(m_device->graphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS) {
            break;
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapchains[] = { m_swapchain->get() };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;
        vkQueuePresentKHR(m_device->presentQueue(), &presentInfo);

        currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
    }

    vkDeviceWaitIdle(m_device->get());
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
