#include "vulkan/window.h"

#include "vulkan/command_pool.h"
#include "vulkan/device.h"
#include "vulkan/instance.h"
#include "vulkan/swapchain.h"

#include <iostream>
#include <limits>

const size_t kMaxFramesInFlight = 2;

namespace scin {

namespace vk {

Window::Window(std::shared_ptr<Instance> instance) :
    instance_(instance),
    width_(-1),
    height_(-1),
    window_(nullptr),
    surface_(VK_NULL_HANDLE) {
}

Window::~Window() {
}

bool Window::Create(int width, int height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(width, height,
            "ScintillatorSynth", nullptr, nullptr);
    if (glfwCreateWindowSurface(instance_->get(), window_, nullptr, &surface_)
            != VK_SUCCESS) {
        std::cerr << "failed to create surface" << std::endl;
        return false;
    }

    width_ = width;
    height_ = height;

    return true;
}

bool Window::CreateSyncObjects(Device* device) {
    image_available_semaphores_.resize(kMaxFramesInFlight);
    render_finished_semaphores_.resize(kMaxFramesInFlight);
    in_flight_fences_.resize(kMaxFramesInFlight);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (vkCreateSemaphore(device->get(), &semaphore_info, nullptr,
                &image_available_semaphores_[i]) != VK_SUCCESS) {
            return false;
        }
        if (vkCreateSemaphore(device->get(), &semaphore_info, nullptr,
                &render_finished_semaphores_[i]) != VK_SUCCESS) {
            return false;
        }
        if (vkCreateFence(device->get(), &fence_info, nullptr,
                &in_flight_fences_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

void Window::Run(Device* device, Swapchain* swapchain,
        CommandPool* command_pool) {
    size_t current_frame = 0;

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        vkWaitForFences(device->get(), 1, &in_flight_fences_[current_frame],
                VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkResetFences(device->get(), 1, &in_flight_fences_[current_frame]);

        uint32_t image_index;
        vkAcquireNextImageKHR(
                device->get(),
                swapchain->get(),
                std::numeric_limits<uint64_t>::max(),
                image_available_semaphores_[current_frame],
                VK_NULL_HANDLE,
                &image_index);

        VkSemaphore wait_semaphores[] = {
            image_available_semaphores_[current_frame]
        };
        VkPipelineStageFlags wait_stages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkCommandBuffer command_buffer = command_pool->command_buffer(
                image_index);
        VkSemaphore signal_semaphores[] = {
            render_finished_semaphores_[current_frame]
        };
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;
        if (vkQueueSubmit(device->graphics_queue(), 1, &submit_info,
                in_flight_fences_[current_frame]) != VK_SUCCESS) {
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

void Window::DestroySyncObjects(Device* device) {
    for (auto semaphore : image_available_semaphores_) {
        vkDestroySemaphore(device->get(), semaphore, nullptr);
    }
    image_available_semaphores_.clear();

    for (auto semaphore : render_finished_semaphores_) {
        vkDestroySemaphore(device->get(), semaphore, nullptr);
    }
    render_finished_semaphores_.clear();

    for (auto fence : in_flight_fences_) {
        vkDestroyFence(device->get(), fence, nullptr);
    }
    in_flight_fences_.clear();
}

void Window::Destroy() {
    vkDestroySurfaceKHR(instance_->get(), surface_, nullptr);
    glfwDestroyWindow(window_);
}

}    // namespace vk

}    // namespace scin

