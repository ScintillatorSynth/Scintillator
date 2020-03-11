#include "comp/Window.hpp"

#include "comp/Canvas.hpp"
#include "comp/Compositor.hpp"
#include "comp/FrameTimer.hpp"
#include "comp/Offscreen.hpp"
#include "comp/RenderSync.hpp"
#include "comp/StageManager.hpp"
#include "comp/Swapchain.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"
#include "vulkan/Instance.hpp"

#include "spdlog/spdlog.h"

#include <chrono>
#include <limits>

namespace scin { namespace comp {

Window::Window(std::shared_ptr<vk::Instance> instance, std::shared_ptr<vk::Device> device, int width, int height,
               bool keepOnTop, int frameRate):
    m_instance(instance),
    m_device(device),
    m_width(width),
    m_height(height),
    m_keepOnTop(keepOnTop),
    m_frameRate(frameRate),
    m_directRendering(frameRate < 0),
    m_window(nullptr),
    m_surface(VK_NULL_HANDLE),
    m_swapchain(new Swapchain(device)),
    m_renderSync(new RenderSync(device)),
    m_frameTimer(new FrameTimer(frameRate)),
    m_stop(false) {}

Window::~Window() {}

bool Window::create() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, m_keepOnTop ? GLFW_TRUE : GLFW_FALSE);
    m_window = glfwCreateWindow(m_width, m_height, "ScintillatorSynth", nullptr, nullptr);
    if (glfwCreateWindowSurface(m_instance->get(), m_window, nullptr, &m_surface) != VK_SUCCESS) {
        const char* errorText = nullptr;
        int code = glfwGetError(&errorText);
        spdlog::error("Window failed to create surface, glfw error code {}, string {}", code, errorText);
        return false;
    }

    if (!m_swapchain->create(this, m_directRendering)) {
        spdlog::error("Window failed to create swapchain.");
        return false;
    }

    if (!m_renderSync->create(1, true)) {
        spdlog::error("Window failed to create rendering synchronization primitives.");
        return false;
    }

    if (!m_directRendering) {
        m_offscreen.reset(new Offscreen(m_device, m_width, m_height, m_frameRate));
        if (!m_offscreen->create(m_swapchain->numberOfImages() + 1)) {
            spdlog::error("Window failed to create Offscreen rendering environment.");
            return false;
        }

        if (!m_offscreen->supportSwapchain(m_swapchain, m_renderSync)) {
            spdlog::error("Window failed to create Swapchain support for Offscreen rendering.");
            return false;
        }
    }

    return true;
}

void Window::run(std::shared_ptr<comp::Compositor> compositor) {
    if (m_directRendering) {
        runDirectRendering(compositor);
    } else {
        runFixedFrameRate(compositor);
    }
}

void Window::destroy() {
    if (!m_directRendering) {
        m_offscreen->destroy();
    }
    m_commandBuffers = nullptr;
    m_renderSync->destroy();
    m_swapchain->destroy();
    vkDestroySurfaceKHR(m_instance->get(), m_surface, nullptr);
    glfwDestroyWindow(m_window);
}

std::shared_ptr<Canvas> Window::canvas() {
    if (m_directRendering) {
        return m_swapchain->canvas();
    }
    return m_offscreen->canvas();
}

std::shared_ptr<Offscreen> Window::offscreen() { return m_offscreen; }

std::shared_ptr<const FrameTimer> Window::frameTimer() {
    if (m_directRendering) {
        return m_frameTimer;
    }
    return m_offscreen->frameTimer();
}

void Window::runDirectRendering(std::shared_ptr<comp::Compositor> compositor) {
    spdlog::info("Window starting direct rendering loop.");
    m_frameTimer->start();

    while (!m_stop && !glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // Wait for the next frame to be finished with the last render.
        m_renderSync->waitForFrame(0);

        // Ask Vulkan which is the next available image in the swapchain, wait indefinitely for it to become
        // available.
        uint32_t imageIndex = m_renderSync->acquireNextImage(0, m_swapchain.get());

        m_frameTimer->markFrame();

        m_commandBuffers = compositor->prepareFrame(imageIndex, m_frameTimer->elapsedTime());

        VkSemaphore imageAvailable[] = { m_renderSync->imageAvailable(0) };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore renderFinished[] = { m_renderSync->renderFinished(0) };
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = imageAvailable;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffers[] = { m_commandBuffers->buffer(imageIndex) };
        submitInfo.pCommandBuffers = commandBuffers;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = renderFinished;

        // Reset the fence so that it can be signaled by the render completion again.
        m_renderSync->resetFrame(0);

        // Submits the command buffer to the queue. Won't start the buffer until the image is marked as available by
        // the imageAvailable semaphore, and when the render is finished it will signal the renderFinished semaphore
        // on the device.
        if (vkQueueSubmit(m_device->graphicsQueue(), 1, &submitInfo, m_renderSync->frameRendering(0)) != VK_SUCCESS) {
            spdlog::error("Window failed to submit command buffer to graphics queue.");
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

        // Submit any pending transfer operations.
        compositor->stageManager()->submitTransferCommands(m_device->graphicsQueue());
    }

    vkDeviceWaitIdle(m_device->get());
    spdlog::info("Window exiting direct rendering loop.");
}

void Window::runFixedFrameRate(std::shared_ptr<Compositor> compositor) {
    spdlog::info("Window starting offscreen rendering loop.");
    m_offscreen->runThreaded(compositor);

    std::chrono::high_resolution_clock::time_point lastFrame = std::chrono::high_resolution_clock::now();
    while (!m_stop && !glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        std::this_thread::sleep_until(lastFrame + std::chrono::milliseconds(200));

        m_renderSync->waitForFrame(0);
        uint32_t imageIndex = m_renderSync->acquireNextImage(0, m_swapchain.get());

        // Offscreen will configure the submit to clear the fence again, so we reset it here.
        m_renderSync->resetFrame(0);
        m_offscreen->requestSwapchainBlit(imageIndex);

        lastFrame = std::chrono::high_resolution_clock::now();
    }
    m_offscreen->stop();

    spdlog::info("Window exiting offscreen rendering loop.");
}

} // namespace comp

} // namespace scin
