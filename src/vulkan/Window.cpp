#include "vulkan/Window.hpp"

#include "vulkan/Buffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Instance.hpp"
#include "vulkan/Swapchain.hpp"
#include "vulkan/Uniform.hpp"

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

bool Window::setDevice(std::shared_ptr<Device> device) {
    m_device = device;
}

bool Window::createSyncObjects() {
    m_imageAvailableSemaphores.resize(kMaxFramesInFlight);
    m_renderFinishedSemaphores.resize(kMaxFramesInFlight);
    m_inFlightFences.resize(kMaxFramesInFlight);

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

void Window::run() {
    size_t current_frame = 0;
    auto startTime = std::chrono::steady_clock::now();

    while (!m_stop && !glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        vkWaitForFences(device->get(), 1, &m_inFlightFences[current_frame], VK_TRUE,
                        std::numeric_limits<uint64_t>::max());

        uint32_t image_index;
        vkAcquireNextImageKHR(device->get(), swapchain->get(), std::numeric_limits<uint64_t>::max(),
                              m_imageAvailableSemaphores[current_frame], VK_NULL_HANDLE, &image_index);

        // Update time uniform.
        auto currentTime = std::chrono::steady_clock::now();
        GlobalUniform gbo;
        gbo.time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        std::shared_ptr<HostBuffer> uniformBuffer = uniform->buffer(image_index);
        uniformBuffer->copyToGPU(&gbo);

        VkSemaphore wait_semaphores[] = { m_imageAvailableSemaphores[current_frame] };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkCommandBuffer command_buffer = VK_NULL_HANDLE; // FIXME command_pool->command_buffer(image_index);
        VkSemaphore signal_semaphores[] = { m_renderFinishedSemaphores[current_frame] };

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        vkResetFences(device->get(), 1, &m_inFlightFences[current_frame]);

        if (vkQueueSubmit(device->graphics_queue(), 1, &submit_info, m_inFlightFences[current_frame]) != VK_SUCCESS) {
            break;
        }

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;
        VkSwapchainKHR swapchains[] = { swapchain->get() };
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &image_index;
        present_info.pResults = nullptr;
        vkQueuePresentKHR(device->present_queue(), &present_info);

        current_frame = (current_frame + 1) % kMaxFramesInFlight;
    }

    vkDeviceWaitIdle(device->get());
}

void Window::destroySyncObjects(Device* device) {
    for (auto semaphore : m_imageAvailableSemaphores) {
        vkDestroySemaphore(device->get(), semaphore, nullptr);
    }
    m_imageAvailableSemaphores.clear();

    for (auto semaphore : m_renderFinishedSemaphores) {
        vkDestroySemaphore(device->get(), semaphore, nullptr);
    }
    m_renderFinishedSemaphores.clear();

    for (auto fence : m_inFlightFences) {
        vkDestroyFence(device->get(), fence, nullptr);
    }
    m_inFlightFences.clear();
}

void Window::destroy() {
    vkDestroySurfaceKHR(m_instance->get(), m_surface, nullptr);
    glfwDestroyWindow(m_window);
}

} // namespace vk

} // namespace scin
