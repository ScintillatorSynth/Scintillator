#include "vulkan/Offscreen.hpp"

#include "Compositor.hpp"

#include "av/BufferPool.hpp"
#include "av/Encoder.hpp"

#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Framebuffer.hpp"
#include "vulkan/ImageSet.hpp"
#include "vulkan/RenderSync.hpp"
#include "vulkan/Swapchain.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Offscreen::Offscreen(std::shared_ptr<Device> device):
    m_device(device),
    m_quit(false),
    m_framebuffer(new Framebuffer(device)),
    m_renderSync(new RenderSync(device)),
    m_commandPool(new CommandPool(device)),
    m_sourceStates({ kEmpty, kEmpty }) {}

Offscreen::~Offscreen() { destroy(); }

bool Offscreen::create(std::shared_ptr<Compositor> compositor, int width, int height, size_t numberOfImages) {
    m_bufferPool.reset(new scin::av::BufferPool(width, height));
    m_compositor = compositor;
    m_numberOfImages = std::max(numberOfImages, 2ul);

    spdlog::info("creating Offscreen renderer with {} images.", m_numberOfImages);

    if (!m_framebuffer->create(width, height, m_numberOfImages)) {
        spdlog::error("Offscreen failed to create framebuffer of width: {}, height: {}, images: {}", width, height,
                      m_numberOfImages);
        return false;
    }

    if (!m_renderSync->create(m_numberOfImages, false)) {
        spdlog::error("Offscreen faild to create the render synchronization primitives.");
        return false;
    }

    // Prepare for readback by allocating GPU memory for readback images, checking for efficient copy operations from
    // the framebuffer to those readback images, and building command buffers to do the actual readback.
    m_readbackImages.reset(new ImageSet(m_device));
    if (!m_readbackImages->createHostCoherent(width, height, m_numberOfImages)) {
        spdlog::error("Offscreen failed to create {} readback images.", m_numberOfImages);
        return false;
    }

    // Check if the physical device supports from and to the required formats.
    m_readbackSupportsBlit = true;
    VkFormatProperties format;
    vkGetPhysicalDeviceFormatProperties(m_device->getPhysical(), m_framebuffer->format(), &format);
    if (!(format.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
        spdlog::warn("Offscreen swapchain surface doesn't support blit source, readback will be slow.");
        m_readbackSupportsBlit = false;
    }
    vkGetPhysicalDeviceFormatProperties(m_device->getPhysical(), m_readbackImages->format(), &format);
    if (!(format.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
        spdlog::warn("Offscreen readback image format doesn't support blit destination, readback will be slow.");
        m_readbackSupportsBlit = false;
    }

    // Build the readback commands.
    if (!m_commandPool->create()) {
        spdlog::error("Offscreen failed to create command pool.");
        return false;
    }
    m_readbackCommands = m_commandPool->createBuffers(m_numberOfImages, true);
    if (!m_readbackCommands) {
        spdlog::error("Offscreen failed to create command buffers.");
        return false;
    }
    for (auto i = 0; i < m_numberOfImages; ++i) {
        if (!writeBlitCommands(m_readbackCommands, i, width, height, m_framebuffer->image(i),
                               m_readbackImages->get()[i])) {
            spdlog::error("Offscreen failed to create readback command buffers.");
            return false;
        }
    }

    m_render = false;
    m_frameRate = 0;
    m_renderThread = std::thread(&Offscreen::threadMain, this, compositor);
    return true;
}

bool Offscreen::createSwapchainSources(Swapchain* swapchain) {
    int width = swapchain->extent().width;
    int height = swapchain->extent().height;
    m_swapSources.reset(new ImageSet(m_device));

    if (!m_swapSources->createDeviceLocal(width, height, 2, false)) {
        spdlog::error("Offscreen failed creating swapchain source images.");
        return false;
    }

    // Build the transfer from framebuffer to swapchain sources command buffers.
    for (auto i = 0; i < m_numberOfImages; ++i) {
        std::shared_ptr<CommandBuffer> buffer = m_commandPool->createBuffers(2, true);
        if (!buffer) {
            spdlog::error("Offscreen failed creating swapchain source blit command buffers.");
            return false;
        }
        m_sourceBlitCommands.push_back(buffer);

        // The jth command buffer will blit from the ith framebuffer image to the jth swapchain source image.
        for (auto j = 0; j < 2; ++j) {
            if (!writeBlitCommands(buffer, j, width, height, m_framebuffer->image(i), m_swapSources->get()[i])) {
                spdlog::error("Offscreen failed writing swapchain source blit command buffers.");
                return false;
            }
        }
    }

    // Build the transfer from swapchain sources -> swapchain image command buffers.
    for (auto i = 0; i < 2; ++i) {
        std::shared_ptr<CommandBuffer> buffer = m_commandPool->createBuffers(swapchain->numberOfImages(), true);
        if (!buffer) {
            spdlog::error("Offscreen failed creating swapchain blit command buffers.");
            return false;
        }
        m_swapBlitCommands.push_back(buffer);

        // The jth command buffer will blit from the ith swap source image to the jth swapchain image.
        for (auto j = 0; j < swapchain->numberOfImages(); ++j) {
            if (!writeBlitCommands(buffer, j, width, height, m_swapSources->get()[i], swapchain->images()->get()[j])) {
                spdlog::error("Ofscreen failed writing swapchain blit command buffers.");
                return false;
            }
        }
    }

    // We know there will be a swapchain requesting frames so start a render request for the first image.
    m_sourceStates[0] = kRequested;
    return true;
}

void Offscreen::addEncoder(std::shared_ptr<scin::av::Encoder> encoder) {
    std::lock_guard<std::mutex> lock(m_encodersMutex);
    m_encoders.push_back(encoder);
}

std::shared_ptr<CommandBuffer> Offscreen::getSwapchainBlit() {
    int sourceIndex = -1;
    {
        std::lock_guard<std::mutex> lock(m_swapMutex);
        // If there's a ready image, we mark it as in use, and change the other image to requested (to request the
        // next frame).
        if (m_sourceStates[0] == kReady) {
            sourceIndex = 0;
        } else if (m_sourceStates[1] == kReady) {
            sourceIndex = 1;
        }

        // Ok, no ready image, look for an already in-use image.
        if (sourceIndex == -1) {
            if (m_sourceStates[0] == kInUse) {
                sourceIndex = 0;
            } else if (m_sourceStates[1] == kInUse) {
                sourceIndex = 1;
            }
        }

        // Make sure the next frame is requested, if we haven't already.
        if (sourceIndex >= 0) {
            int otherIndex = (sourceIndex + 1) % 2;
            if (m_sourceStates[otherIndex] == kEmpty || m_sourceStates[otherIndex] == kInUse) {
                m_sourceStates[otherIndex] = kRequested;
            }
        }
    }

    if (sourceIndex == -1) {
        return nullptr;
    }

    return m_swapBlitCommands[sourceIndex];
}

void Offscreen::destroy() {}

std::shared_ptr<Canvas> Offscreen::canvas() { return m_framebuffer->canvas(); }

void Offscreen::threadMain(std::shared_ptr<Compositor> compositor) {
    spdlog::info("Offscreen render thread starting up.");

    double time = 0.0;
    size_t frameNumber = 0;
    size_t frameIndex = 0;

    m_commandBuffers.reserve(m_numberOfImages);
    for (auto i = 0; i < m_numberOfImages; ++i) {
        m_pendingSwapchainBlits.push_back(-1);
    }

    while (!m_quit) {
        double deltaTime = 0.0;
        bool flush = false;
        {
            std::unique_lock<std::mutex> lock(m_renderMutex);
            m_renderCondition.wait(lock, [this] { return m_quit || m_render; });
            if (m_quit) {
                break;
            }
            if (!m_render) {
                continue;
            }
            // If framerate is 0 then we turn the render flag back off to block again after this iteration.
            if (m_frameRate == 0) {
                m_render = false;
                flush = true;
            }
            deltaTime = m_deltaTime;
        }

        // Wait for rendering/blitting to complete on this frame.
        m_renderSync->waitForFrame(frameIndex);

        processPendingBlits(frameIndex);

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
                    encodeRequests.push_back(encode);
                    ++it;
                } else {
                    it = m_encoders.erase(it);
                }
            }
        }

        // If there's a request for at least one encode add the command buffer to blit/copy to the readback buffer.
        if (encodeRequests.size()) {
            commandBuffers.push_back(m_readbackCommands->buffer(frameIndex));
        }

        // If we should update the swapchain source image also add that blit command buffer.
        int swapSourceIndex = -1;
        {
            std::lock_guard<std::mutex> lock(m_swapMutex);
            if (m_sourceStates[0] == kRequested) {
                swapSourceIndex = 0;
                m_sourceStates[0] = kPipelined;
            } else if (m_sourceStates[1] == kRequested) {
                swapSourceIndex = 1;
                m_sourceStates[1] = kPipelined;
            }
        }
        m_pendingSwapchainBlits[frameIndex] = swapSourceIndex;

        if (swapSourceIndex >= 0) {
            commandBuffers.push_back(m_sourceBlitCommands[frameIndex]->buffer(swapSourceIndex));
        }

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
            processPendingBlits(frameIndex);
        }

        time += deltaTime;
        ++frameNumber;
        frameIndex = frameNumber % m_numberOfImages;
    }
}

void Offscreen::processPendingBlits(size_t frameIndex) {
    int swapIndex = m_pendingSwapchainBlits[frameIndex];
    if (swapIndex >= 0) {
        std::lock_guard<std::mutex> lock(m_swapMutex);
        m_sourceStates[swapIndex] = kReady;
    }
    m_pendingSwapchainBlits[frameIndex] = -1;

    // terrible name for m_pendingEncodes, maybe m_pendingEncodes?
    if (m_pendingEncodes[frameIndex].size()) {
        // ffmpeg can decide on a buffer recycling option.
        std::shared_ptr<scin::av::Buffer> avBuffer = m_bufferPool->getBuffer();
        // Readback the first frame request from the GPU.
        m_readbackImages->readbackImage(frameIndex, avBuffer.get());
        for (auto callback : m_pendingEncodes[frameIndex]) {
            callback(avBuffer);
        }
        m_pendingEncodes[frameIndex].clear();
    }
}

bool Offscreen::writeBlitCommands(std::shared_ptr<CommandBuffer> commandBuffer, size_t bufferIndex, int width,
                                  int height, VkImage sourceImage, VkImage destinationImage) {
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
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
    offset.x = width;
    offset.y = height;
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

    // ?? Do we need to transition things back?

    if (vkEndCommandBuffer(commandBuffer->buffer(bufferIndex)) != VK_SUCCESS) {
        spdlog::info("Offscreen failed ending readback command buffer.");
        return false;
    }

    return true;
}

} // namespace vk

} // namespace scin
