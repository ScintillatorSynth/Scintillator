#include "vulkan/Window.hpp"

#include "Compositor.hpp"
#include "av/Context.hpp"
#include "vulkan/Canvas.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/ImageSet.hpp"
#include "vulkan/Instance.hpp"
#include "vulkan/Swapchain.hpp"

#include "spdlog/spdlog.h"

#include <limits>

namespace scin { namespace vk {

const size_t kFramePeriodWindowSize = 60;

Window::Window(std::shared_ptr<Instance> instance, int width, int height, bool keepOnTop, int frameRate):
    m_instance(instance),
    m_width(width),
    m_height(height),
    m_keepOnTop(keepOnTop),
    m_frameRate(frameRate),
    m_window(nullptr),
    m_surface(VK_NULL_HANDLE),
    m_stop(false),
    m_periodSum(0.0),
    m_lateFrames(0),
    m_imageAvailable(VK_NULL_HANDLE),
    m_renderFinished(VK_NULL_HANDLE),
    m_frameRendering(VK_NULL_HANDLE),
    m_readbackSupportsBlit(false) {}

Window::~Window() {}

bool Window::create() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, m_keepOnTop ? GLFW_TRUE : GLFW_FALSE);
    m_window = glfwCreateWindow(m_width, m_height, "ScintillatorSynth", nullptr, nullptr);
    if (glfwCreateWindowSurface(m_instance->get(), m_window, nullptr, &m_surface) != VK_SUCCESS) {
        spdlog::error("failed to create surface");
        return false;
    }

    return true;
}

bool Window::createSwapchain(std::shared_ptr<Device> device) {
    m_device = device;
    m_swapchain.reset(new Swapchain(m_device));
    // Request FIFO queue submission if we are in free-running mode (frame rate < 0).
    if (!m_swapchain->create(this, m_frameRate < 0)) {
        spdlog::error("Window failed to create swapchain.");
        return false;
    }

    // Prepare for readback by allocating GPU memory for readback images, checking for efficient copy operations from
    // the swapbuffer to those readback images, and building command buffers to do the actual readback.
    m_readbackImages.reset(new ImageSet(m_device));
    if (!m_readbackImages->createHostTransferTarget(m_width, m_height, m_swapchain->numberOfImages())) {
        spdlog::error("Window failed to create readback images.");
        return false;
    }

    // Check if the physical device supports from and to the required formats.
    m_readbackSupportsBlit = true;
    VkFormatProperties format;
    vkGetPhysicalDeviceFormatProperties(m_device->getPhysical(), m_swapchain->surfaceFormat().format, &format);
    if (!(format.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
        spdlog::warn("Window swapchain surface doesn't support blit source, readback will be slow.");
        m_readbackSupportsBlit = false;
    }
    vkGetPhysicalDeviceFormatProperties(m_device->getPhysical(), m_readbackImages->format(), &format);
    if (!(format.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
        spdlog::warn("Window readback image format doesn't support blit destination, readback will be slow.");
        m_readbackSupportsBlit = false;
    }

    // Build the readback commands.
    m_commandPool.reset(new CommandPool(m_device));
    if (!m_commandPool->create()) {
        spdlog::error("Window failed to create command pool.");
        return false;
    }
    m_readbackCommands = m_commandPool->createBuffers(m_swapchain->numberOfImages(), true);
    if (!m_readbackCommands) {
        spdlog::error("Window failed to create command buffers.");
        return false;
    }
    for (auto i = 0; i < m_swapchain->numberOfImages(); ++i) {
        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        // TODO: possible to batch both of these transitions into a single vkCmdPipelineBarrier call, is faster?

        // Transition swapchain image into a transfer source configuration.
        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        memoryBarrier.image = m_swapchain->images()->get()[i];
        memoryBarrier.subresourceRange = range;
        vkCmdPipelineBarrier(m_readbackCommands->buffer(i), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

        // Transition readback image to a transfer destination configuration.
        memoryBarrier.srcAccessMask = 0;
        memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memoryBarrier.image = m_readbackImages->get()[i];
        vkCmdPipelineBarrier(m_readbackCommands->buffer(i), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

        if (m_readbackSupportsBlit) {
            VkOffset3D offset = {};
            offset.x = m_width;
            offset.y = m_height;
            offset.z = 1;
            VkImageBlit blitRegion = {};
            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.layerCount = 1;
            // src and dstOffsets[0] are all zeros.
            blitRegion.srcOffsets[1] = offset;
            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.layerCount = 1;
            blitRegion.dstOffsets[1] = offset;
            vkCmdBlitImage(m_readbackCommands->buffer(i), m_swapchain->images()->get()[i],
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_readbackImages->get()[i],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);
        } else {
            // TODO non-blit copyback.
        }

        // Transition swapchain image back to ideal rendering target configuration.
        memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memoryBarrier.image = m_swapchain->images()->get()[i];
        vkCmdPipelineBarrier(m_readbackCommands->buffer(i), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

        // Transition readback image to a format accessible by the host.
        memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        memoryBarrier.image = m_readbackImages->get()[i];
        vkCmdPipelineBarrier(m_readbackCommands->buffer(i), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
    }


    return true;
}

bool Window::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(m_device->get(), &semaphoreInfo, nullptr, &m_imageAvailable) != VK_SUCCESS) {
        spdlog::error("Window failed to create image available semaphore.");
        return false;
    }
    if (vkCreateSemaphore(m_device->get(), &semaphoreInfo, nullptr, &m_renderFinished) != VK_SUCCESS) {
        spdlog::error("window failed to create render finished semaphore.");
        return false;
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(m_device->get(), &fenceInfo, nullptr, &m_frameRendering) != VK_SUCCESS) {
        spdlog::error("window failed to create render fence.");
        return false;
    }

    return true;
}

void Window::run(std::shared_ptr<Compositor> compositor) {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastFrameTime = m_startTime;
    m_lastReportTime = m_startTime;

    while (!m_stop && !glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // *** At this point in time the CPU could wait on the inflight fence for the frame just submitted (before
        // advancing the frame counter below), and if there was a copy operation requested this would be the optimal
        // time to do queue it as the image will not be needed until its turn in the swapchain again, and presumably
        // the CPU is not needed until it is time to queue another command buffer for the next inflight.


        // TODO: this brings the number of mutexes acquired per frame up to 2 (compositor, window encoders) plus one per
        // active encoder (empty frames queue). Just sayin'. Maybe an atomic or something to indicate that there's
        // anything in there at all, like a list or deque with atomic size (so thread-safe empty queries) could be
        // pretty cool.
        std::vector<std::shared_ptr<scin::av::Frame>> encodeFrames;
        {
            std::lock_guard<std::mutex> lock(m_encodersMutex);
            for (auto it = m_encoders.begin(); it != m_encoders.end(); /* deliberately empty increment */) {
                std::shared_ptr<scin::av::Frame> frame = (*it)->getEmptyFrame();
                // Encoder will return nullptr as a signal to stop sending this encoder filled frames.
                if (frame) {
                    encodeFrames.push_back(frame);
                    ++it;
                } else {
                    it = m_encoders.erase(it);
                }
            }
        }

        // Wait for the next frame to be finished with the last render.
        vkWaitForFences(m_device->get(), 1, &m_frameRendering, VK_TRUE, std::numeric_limits<uint64_t>::max());

        // Ask VulkanKHR which is the next available image in the swapchain, wait indefinitely for it to become
        // available.
        uint32_t imageIndex;
        vkAcquireNextImageKHR(m_device->get(), m_swapchain->get(), std::numeric_limits<uint64_t>::max(),
                              m_imageAvailable, VK_NULL_HANDLE, &imageIndex);

        TimePoint now(std::chrono::high_resolution_clock::now());
        double framePeriod = std::chrono::duration<double, std::chrono::seconds::period>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;

        double meanPeriod =
            m_framePeriods.size() ? m_periodSum / static_cast<double>(m_framePeriods.size()) : framePeriod;
        m_periodSum += framePeriod;
        m_framePeriods.push_back(framePeriod);

        // We consider a frame late when we have at least half of the window of frame times to establish a credible
        // mean, and the period of the frame is more than half again the mean.
        if (m_framePeriods.size() >= kFramePeriodWindowSize / 2 && framePeriod >= (meanPeriod * 1.5)) {
            ++m_lateFrames;
            // Remove the outlier from the average, to avoid biasing our dropped frame detector.
            m_periodSum -= framePeriod;
            m_framePeriods.pop_back();
        }

        while (m_framePeriods.size() > kFramePeriodWindowSize) {
            m_periodSum -= m_framePeriods.front();
            m_framePeriods.pop_front();
        }

        if (std::chrono::duration<double, std::chrono::seconds::period>(now - m_lastReportTime).count() >= 10.0) {
            spdlog::info("mean fps: {:.1f}, late frames: {}", 1.0 / meanPeriod, m_lateFrames);
            m_lateFrames = 0;
            m_lastReportTime = now;
        }

        m_commandBuffers = compositor->prepareFrame(
            imageIndex, std::chrono::duration<double, std::chrono::seconds::period>(now - m_startTime).count());

        VkSemaphore imageAvailable[] = { m_imageAvailable };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore renderFinished[] = { m_renderFinished };
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = imageAvailable;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer = m_commandBuffers->buffer(imageIndex);
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = renderFinished;

        // Reset the fence so that it can be signaled by the render completion again.
        vkResetFences(m_device->get(), 1, &m_frameRendering);

        // Submits the command buffer to the queue. Won't start the buffer until the image is marked as available by
        // the imageAvailable semaphore, and when the render is finished it will signal the renderFinished semaphore
        // on the device.
        if (vkQueueSubmit(m_device->graphicsQueue(), 1, &submitInfo, m_frameRendering) != VK_SUCCESS) {
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
    }

    vkDeviceWaitIdle(m_device->get());
    m_commandBuffers = nullptr;
}

void Window::destroySyncObjects() {
    if (m_imageAvailable != VK_NULL_HANDLE) {
        vkDestroySemaphore(m_device->get(), m_imageAvailable, nullptr);
        m_imageAvailable = VK_NULL_HANDLE;
    }
    if (m_renderFinished != VK_NULL_HANDLE) {
        vkDestroySemaphore(m_device->get(), m_renderFinished, nullptr);
        m_renderFinished = VK_NULL_HANDLE;
    }
    if (m_frameRendering != VK_NULL_HANDLE) {
        vkDestroyFence(m_device->get(), m_frameRendering, nullptr);
        m_frameRendering = VK_NULL_HANDLE;
    }
}

void Window::destroySwapchain() {
    m_readbackImages->destroy();
    m_swapchain->destroy();
}

void Window::destroy() {
    vkDestroySurfaceKHR(m_instance->get(), m_surface, nullptr);
    glfwDestroyWindow(m_window);
}

void Window::addEncoder(std::shared_ptr<scin::av::Encoder> encoder) {
    std::lock_guard<std::mutex> lock(m_encodersMutex);
    m_encoders.push_back(encoder);
}

std::shared_ptr<Canvas> Window::canvas() { return m_swapchain->canvas(); }

} // namespace vk

} // namespace scin
