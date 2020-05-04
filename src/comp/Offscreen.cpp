#include "comp/Offscreen.hpp"

#include "av/Buffer.hpp"
#include "av/BufferPool.hpp"
#include "av/Encoder.hpp"
#include "comp/Compositor.hpp"
#include "comp/FrameTimer.hpp"
#include "comp/RenderSync.hpp"
#include "comp/StageManager.hpp"
#include "comp/Swapchain.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Framebuffer.hpp"
#include "vulkan/Image.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

Offscreen::Offscreen(std::shared_ptr<vk::Device> device, int width, int height, int frameRate):
    m_device(device),
    m_quit(false),
    m_numberOfImages(0),
    m_width(width),
    m_height(height),
    m_frameTimer(new FrameTimer(frameRate)),
    m_framebuffer(new vk::Framebuffer(device)),
    m_renderSync(new RenderSync(device)),
    m_commandPool(new vk::CommandPool(device)),
    m_bufferPool(new scin::av::BufferPool(width, height)),
    m_render(false),
    m_swapBlitRequested(false),
    m_stagingRequested(false),
    m_swapchainImageIndex(0),
    m_frameRate(frameRate),
    m_deltaTime(0) {
    if (frameRate > 0) {
        m_deltaTime = 1.0 / static_cast<double>(frameRate);
        m_snapShotMode = false;
    } else {
        m_snapShotMode = true;
    }
}

Offscreen::~Offscreen() { destroy(); }

bool Offscreen::create(size_t numberOfImages) {
    m_numberOfImages = std::max(numberOfImages, static_cast<size_t>(2));

    spdlog::info("creating Offscreen renderer with {} images.", m_numberOfImages);

    if (!m_framebuffer->create(m_width, m_height, m_numberOfImages)) {
        spdlog::error("Offscreen failed to create framebuffer of width: {}, height: {}, images: {}", m_width, m_height,
                      m_numberOfImages);
        return false;
    }

    if (!m_renderSync->create(m_numberOfImages, false)) {
        spdlog::error("Offscreen faild to create the render synchronization primitives.");
        return false;
    }

    // Prepare for readback by allocating GPU memory for readback images, checking for efficient copy operations from
    // the framebuffer to those readback images, and building command buffers to do the actual readback.
    for (auto i = 0; i < m_numberOfImages; ++i) {
        std::shared_ptr<vk::HostImage> image(new vk::HostImage(m_device));
        if (!image->create(m_width, m_height)) {
            spdlog::error("Offscreen failed to create {} readback images.", m_numberOfImages);
            return false;
        }
        m_readbackImages.push_back(image);
    }

    // Check if the physical device supports from and to the required formats.
    m_readbackSupportsBlit = true;
    VkFormatProperties format;
    vkGetPhysicalDeviceFormatProperties(m_device->physical(), m_framebuffer->format(), &format);
    if (!(format.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
        spdlog::warn("Offscreen swapchain surface doesn't support blit source, readback will be slow.");
        m_readbackSupportsBlit = false;
    }
    vkGetPhysicalDeviceFormatProperties(m_device->physical(), m_readbackImages[0]->format(), &format);
    if (!(format.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
        spdlog::warn("Offscreen readback image format doesn't support blit destination, readback will be slow.");
        m_readbackSupportsBlit = false;
    }

    // Build the readback commands.
    if (!m_commandPool->create()) {
        spdlog::error("Offscreen failed to create command pool.");
        return false;
    }
    m_readbackCommands.reset(new vk::CommandBuffer(m_device, m_commandPool));
    if (!m_readbackCommands->create(m_numberOfImages, true)) {
        spdlog::error("Offscreen failed to create command buffers.");
        return false;
    }
    if (m_readbackSupportsBlit) {
        for (auto i = 0; i < m_numberOfImages; ++i) {
            if (!writeBlitCommands(m_readbackCommands, i, m_framebuffer->image(i), m_readbackImages[i]->get(),
                                   VK_IMAGE_LAYOUT_UNDEFINED)) {
                spdlog::error("Offscreen failed to create readback command buffers.");
                return false;
            }
        }
    } else {
        for (auto i = 0; i < m_numberOfImages; ++i) {
            if (!writeCopyCommands(m_readbackCommands, i, m_framebuffer->image(i), m_readbackImages[i]->get())) {
                spdlog::error("Offscreen failed to create readback command buffers.");
                return false;
            }
        }
    }

    return true;
}

bool Offscreen::supportSwapchain(std::shared_ptr<Swapchain> swapchain, std::shared_ptr<RenderSync> swapRenderSync) {
    m_swapchain = swapchain;
    m_swapRenderSync = swapRenderSync;

    // Build the transfer from framebuffer to swapchain images command buffers.
    for (auto i = 0; i < m_numberOfImages; ++i) {
        std::shared_ptr<vk::CommandBuffer> buffer(new vk::CommandBuffer(m_device, m_commandPool));
        if (!buffer->create(swapchain->numberOfImages(), true)) {
            spdlog::error("Offscreen failed creating swapchain source blit command buffers.");
            return false;
        }
        // The jth command buffer will blit from the ith framebuffer image to the jth swapchain image.
        for (auto j = 0; j < swapchain->numberOfImages(); ++j) {
            if (!writeBlitCommands(buffer, j, m_framebuffer->image(i), swapchain->images()[j]->get(),
                                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)) {
                spdlog::error("Offscreen failed writing swapchain source blit command buffers.");
                return false;
            }
        }

        m_swapBlitCommands.push_back(buffer);
    }

    return true;
}

void Offscreen::runThreaded(std::shared_ptr<Compositor> compositor) {
    m_render = true;
    m_renderThread = std::thread(&Offscreen::threadMain, this, compositor);
    m_renderCondition.notify_one();
}

void Offscreen::run(std::shared_ptr<Compositor> compositor) {
    m_render = true;
    m_renderCondition.notify_one();
    threadMain(compositor);
}

void Offscreen::addEncoder(std::shared_ptr<scin::av::Encoder> encoder) {
    std::lock_guard<std::mutex> lock(m_encodersMutex);
    m_encoders.push_back(encoder);
}

void Offscreen::stop() {
    m_quit = true;
    m_renderCondition.notify_one();
    if (m_renderThread.joinable()) {
        m_renderThread.join();
    }
}

void Offscreen::requestSwapchainBlit(uint32_t swapchainImageIndex) {
    {
        std::lock_guard<std::mutex> lock(m_renderMutex);
        if (m_swapBlitRequested) {
            spdlog::error("Offscreen already had swapchain blit requested.");
            return;
        }
        m_swapBlitRequested = true;
        m_swapchainImageIndex = swapchainImageIndex;
    }
    m_renderCondition.notify_one();
}

void Offscreen::advanceFrame(double dt, std::function<void(size_t)> callback) {
    if (!m_snapShotMode) {
        spdlog::error("Offscreen got render request but not in snap shot mode.");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_renderMutex);
        if (m_render) {
            spdlog::warn("Offscreen detects snapshot render already requested, ignoring");
            return;
        }
        m_deltaTime = dt;
        m_flushCallback = callback;
        m_render = true;
    }
    m_renderCondition.notify_one();
}

void Offscreen::destroy() {
    m_swapRenderSync = nullptr;
    m_swapchain = nullptr;
    m_swapBlitCommands.clear();

    m_commandBuffers.clear();
    m_pendingEncodes.clear();
    m_readbackImages.clear();
    m_encoders.clear();
    m_readbackCommands = nullptr;

    m_bufferPool = nullptr;
    m_commandPool = nullptr;
    m_renderSync = nullptr;
    m_framebuffer = nullptr;
}

std::shared_ptr<Canvas> Offscreen::canvas() { return m_framebuffer->canvas(); }

void Offscreen::threadMain(std::shared_ptr<Compositor> compositor) {
    spdlog::info("Offscreen render thread starting up.");

    compositor->stageManager()->setStagingRequested([this] {
        {
            std::lock_guard<std::mutex> lock(m_renderMutex);
            m_stagingRequested = true;
        }
        m_renderCondition.notify_one();
    });

    double time = 0.0;
    size_t frameNumber = 0;
    size_t frameIndex = 0;

    m_commandBuffers.resize(m_numberOfImages);
    for (auto i = 0; i < m_numberOfImages; ++i) {
        m_pendingSwapchainBlits.push_back(-1);
    }
    m_pendingEncodes.resize(m_numberOfImages);

    m_frameTimer->start();

    while (!m_quit) {
        double deltaTime = 0.0;
        bool flush = false;
        bool render = false;
        bool stage = false;
        bool swapBlit = false;
        uint32_t swapImageIndex = 0;
        std::function<void(size_t)> flushCallback;

        {
            std::unique_lock<std::mutex> lock(m_renderMutex);
            m_renderCondition.wait(lock,
                                   [this] { return m_quit || m_swapBlitRequested || m_stagingRequested || m_render; });
            if (m_quit) {
                break;
            }

            render = m_render;
            stage = m_stagingRequested;
            swapBlit = m_swapBlitRequested;
            deltaTime = m_deltaTime;

            if (stage) {
                m_stagingRequested = false;
            }

            if (swapBlit) {
                render = false;
                m_swapBlitRequested = false;
                swapImageIndex = m_swapchainImageIndex;
            }

            // If framerate is 0 then we turn the render flag back off to block again after this iteration.
            if (render && m_frameRate == 0) {
                m_render = false;
                flush = true;
                m_deltaTime = 0.0;
                flushCallback = m_flushCallback;
            }
        }

        if (stage) {
            compositor->stageManager()->submitTransferCommands(m_device->graphicsQueue());
        }
        if (swapBlit) {
            blitAndPresent(frameIndex, swapImageIndex);
            continue;
        }
        if (!render) {
            continue;
        }

        // Wait for rendering/blitting to complete on this frame.
        m_renderSync->waitForFrame(frameIndex);

        m_frameTimer->markFrame();

        processPendingEncodes(frameIndex);

        // OK, we now consider the contents of the framebuffer (and any blit targets) as subject to GPU mutation.
        m_commandBuffers[frameIndex] = compositor->prepareFrame(frameIndex, time);

        std::vector<VkCommandBuffer> commandBuffers;
        commandBuffers.push_back(m_commandBuffers[frameIndex]->buffer(frameIndex));

        // Build list of active encoder frames we need to fill, if any. If we are flushing this frame we will process
        // this list before starting another render. If pipelined, we add this to the list of destinations once we
        // clear the fence at the end of the pipeline.
        std::vector<std::function<void(std::shared_ptr<scin::av::Buffer>)>> encodeRequests;
        {
            std::lock_guard<std::mutex> lock(m_encodersMutex);
            for (auto it = m_encoders.begin(); it != m_encoders.end(); /* deliberately empty increment */) {
                scin::av::Encoder::SendBuffer encode;
                if ((*it)->queueEncode(time, frameNumber, encode)) {
                    ++it;
                } else {
                    it = m_encoders.erase(it);
                }
                encodeRequests.push_back(encode);
            }
        }

        // If there's a request for at least one encode add the command buffer to blit/copy to the readback buffer.
        if (encodeRequests.size()) {
            commandBuffers.push_back(m_readbackCommands->buffer(frameIndex));
        }
        m_pendingEncodes[frameIndex] = encodeRequests;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.commandBufferCount = commandBuffers.size();
        submitInfo.pCommandBuffers = commandBuffers.data();
        submitInfo.signalSemaphoreCount = 0;

        m_renderSync->resetFrame(frameIndex);

        if (vkQueueSubmit(m_device->graphicsQueue(), 1, &submitInfo, m_renderSync->frameRendering(frameIndex))
            != VK_SUCCESS) {
            spdlog::error("Offscreen failed to submit command buffer to graphics queue.");
            break;
        }

        // If we should flush this frame, wait for the fence now, otherwise consider this frame wrapped.
        if (flush) {
            m_renderSync->waitForFrame(frameIndex);
            processPendingEncodes(frameIndex);
            if (flushCallback) {
                m_flushCallback(frameNumber);
            }
        }

        time += deltaTime;
        ++frameNumber;
        frameIndex = frameNumber % m_numberOfImages;
    }

    vkDeviceWaitIdle(m_device->get());
    spdlog::info("Offscreen render thread terminating.");
}

void Offscreen::processPendingEncodes(size_t frameIndex) {
    if (m_pendingEncodes[frameIndex].size()) {
        std::shared_ptr<scin::av::Buffer> avBuffer = m_bufferPool->getBuffer();
        std::memcpy(avBuffer->data(), m_readbackImages[frameIndex]->mappedAddress(), avBuffer->size());
        for (auto callback : m_pendingEncodes[frameIndex]) {
            callback(avBuffer);
        }
        m_pendingEncodes[frameIndex].clear();
    }
}

bool Offscreen::writeCopyCommands(std::shared_ptr<vk::CommandBuffer> commandBuffer, size_t bufferIndex,
                                  VkImage sourceImage, VkImage destinationImage) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(commandBuffer->buffer(bufferIndex), &beginInfo) != VK_SUCCESS) {
        spdlog::error("Offscreen failed beginning readback command buffer.");
        return false;
    }

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkImageMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    memoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    memoryBarrier.image = sourceImage;
    memoryBarrier.subresourceRange = range;
    vkCmdPipelineBarrier(commandBuffer->buffer(bufferIndex), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

    memoryBarrier.srcAccessMask = 0;
    memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    memoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    memoryBarrier.image = destinationImage;
    vkCmdPipelineBarrier(commandBuffer->buffer(bufferIndex), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

    VkImageCopy imageCopy = {};
    imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopy.srcSubresource.layerCount = 1;
    imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopy.dstSubresource.layerCount = 1;
    imageCopy.extent.width = m_width;
    imageCopy.extent.height = m_height;
    imageCopy.extent.depth = 1;
    vkCmdCopyImage(commandBuffer->buffer(bufferIndex), sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

    if (vkEndCommandBuffer(commandBuffer->buffer(bufferIndex)) != VK_SUCCESS) {
        spdlog::error("Offscreen failed ending readback command buffer.");
        return false;
    }

    return true;
}

bool Offscreen::writeBlitCommands(std::shared_ptr<vk::CommandBuffer> commandBuffer, size_t bufferIndex,
                                  VkImage sourceImage, VkImage destinationImage, VkImageLayout destinationLayout) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(commandBuffer->buffer(bufferIndex), &beginInfo) != VK_SUCCESS) {
        spdlog::error("Offscreen failed beginning readback command buffer.");
        return false;
    }

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkImageMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    memoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    memoryBarrier.image = sourceImage;
    memoryBarrier.subresourceRange = range;
    vkCmdPipelineBarrier(commandBuffer->buffer(bufferIndex), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

    memoryBarrier.srcAccessMask = 0;
    memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    memoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    memoryBarrier.image = destinationImage;
    vkCmdPipelineBarrier(commandBuffer->buffer(bufferIndex), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

    VkOffset3D offset = {};
    offset.x = m_width;
    offset.y = m_height;
    offset.z = 1;
    VkImageBlit blitRegion = {};
    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.layerCount = 1;
    // srcOffsets[0] and dstOffsets[0] are all zeros.
    blitRegion.srcOffsets[1] = offset;
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstOffsets[1] = offset;
    vkCmdBlitImage(commandBuffer->buffer(bufferIndex), sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);

    if (destinationLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memoryBarrier.newLayout = destinationLayout;
        memoryBarrier.image = destinationImage;
        vkCmdPipelineBarrier(commandBuffer->buffer(bufferIndex), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
    }

    if (vkEndCommandBuffer(commandBuffer->buffer(bufferIndex)) != VK_SUCCESS) {
        spdlog::error("Offscreen failed ending readback command buffer.");
        return false;
    }

    return true;
}

bool Offscreen::blitAndPresent(size_t frameIndex, uint32_t swapImageIndex) {
    VkSemaphore imageAvailable[] = { m_swapRenderSync->imageAvailable(0) };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore renderFinished[] = { m_swapRenderSync->renderFinished(0) };
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = imageAvailable;
    submitInfo.pWaitDstStageMask = waitStages;
    VkCommandBuffer commandBuffer = m_swapBlitCommands[frameIndex]->buffer(swapImageIndex);
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = renderFinished;

    if (vkQueueSubmit(m_device->graphicsQueue(), 1, &submitInfo, m_swapRenderSync->frameRendering(0)) != VK_SUCCESS) {
        spdlog::error("Offscreen failed to submit swapchain blit command buffer to graphics queue.");
        return false;
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = renderFinished;
    VkSwapchainKHR swapchains[] = { m_swapchain->get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &swapImageIndex;
    presentInfo.pResults = nullptr;
    vkQueuePresentKHR(m_device->presentQueue(), &presentInfo);

    return true;
}

} // namespace comp

} // namespace scin
