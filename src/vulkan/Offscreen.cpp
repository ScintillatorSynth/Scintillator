#include "vulkan/Offscreen.hpp"

#include "av/BufferPool.hpp"

#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Framebuffer.hpp"

namespace scin { namespace vk {

Offscreen::Offscreen(std::shared_ptr<Device> device): m_device(device), m_quit(false)
                     m_framebuffer(new Framebuffer(device)), m_renderSync(new RenderSync(device))
                     m_commandPool(new CommandPool(device)) {}

Offscreen::~Offscreen() { destroy(); }

bool Offscreen::create(std::shared_ptr<Compositor> compositor, int width, int height, size_t numberOfImages) {
    m_bufferPool.reset(new BufferPool(width, height));
    m_compositor(compositor);
    m_numberOfImages = std::max(numberOfImages, 2);

    spdlog::info("creating Offscreen renderer with {} images.", m_numberOfImages);

    if (!m_framebuffer->create(width, height, m_numberOfImages)) {
        spdlog::error("Offscreen failed to create framebuffer of width: {}, height: {}, images: {}", width, height,
                m_numberOfImages);
        return false;
    }

    if (!m_renderSync->create(m_numberOfImages)) {
        spdlog::error("Offscreen faild to create the render synchronization primitives.");
        return false;
    }

    // Prepare for readback by allocating GPU memory for readback images, checking for efficient copy operations from
    // the framebuffer to those readback images, and building command buffers to do the actual readback.
    m_readbackImages.reset(new ImageSet(m_device));
    if (!m_readbackImages->createHostTransferTarget(m_width, m_height, m_numberOfImages)) {
        spdlog::error("Window failed to create {} readback images.", m_numberOfImages);
        return false;
    }

    // Check if the physical device supports from and to the required formats.
    m_readbackSupportsBlit = true;
    VkFormatProperties format;
    vkGetPhysicalDeviceFormatProperties(m_device->getPhysical(), m_framebuffer->format(), &format);
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
    if (!m_commandPool->create()) {
        spdlog::error("Window failed to create command pool.");
        return false;
    }
    m_readbackCommands = m_commandPool->createBuffers(m_numberOfImages, true);
    if (!m_readbackCommands) {
        spdlog::error("Window failed to create command buffers.");
        return false;
    }
    for (auto i = 0; i < m_numberOfImages; ++i) {
        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

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
            // srcOffsets[0] and dstOffsets[0] are all zeros.
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

    m_render = false;
    m_frameRate = 0;
    m_renderThread = std::thread(&Offscreen::threadMain, this);
}

void Offscreen::addEncoder(std::shared_ptr<scin::av::Encoder> encoder) {
    std::lock_guard<std::mutex> lock(m_encodersMutex);
    m_encoders.push_back(encoder);
}

void Offscreen::threadMain(int frameRate, std::shared_ptr<Compositor> compositor) {
    spdlog::info("Offscreen render thread starting up.");

    double time = 0.0;
    size_t frameNumber = 0;
    size_t frameIndex = 0;

    m_commandBuffers.reserve(m_numberOfImages);

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

        // Build list of active encoder frames we need to fill, if any. If we are flushing this frame we will process
        // this list before starting another render. If pipelined, we add this to the list of destinations once we
        // clear the fence at the end of the pipeline.
        std::vector<std::function<void(std::shared_ptr<scin::av::Buffer>)>> encodeRequests;
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

        if (encodeFrames.size()) {
            // Add the command buffer to blit/copy to the readback buffer.
        }

        // If we should update the swapchain source image also add that blit command buffer.

        // Send the command buffers, no semaphore needed, only reset the fence associated with this image.

        // If we should flush this frame, wait for the fence now, otherwise consider this frame wrapped.

        time += deltaTime;
        ++frameNumber;
        frameIndex = frameNumber % m_numberOfImages;
    }
}

void Offscreen::processPendingBlits(size_t frameIndex) {
    // ** Update swapchain transfer source if warrented.

    // terrible name for m_pendingEncodes, maybe m_pendingEncodes?
    if (m_pendingEncodes[frameIndex].size()) {
        // ffmpeg can decide on a buffer recycling option.
        std::shared_ptr<scin::av::Buffer> avBuffer = m_bufferPool->getBuffer();
        // Readback the first frame request from the GPU.
        m_readbackImages->readbackFrame(frameIndex, avBuffer.get());
        for (auto callback : m_pendingEncodes[frameIndex]) {
            callback(avBuffer);
        }
        m_pendingEncodes[frameIndex].clear();
    }
}

} // namespace vk

} // namespace scin
