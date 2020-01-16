#include "vulkan/Offscreen.hpp"

#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Framebuffer.hpp"

namespace scin { namespace vk {

Offscreen::Offscreen(std::shared_ptr<Device> device): m_device(device), m_quit(false)
                     m_framebuffer(new Framebuffer(device)), m_renderSync(new RenderSync(device))
                     m_commandPool(new CommandPool(device)) {}

Offscreen::~Offscreen() { destroy(); }

bool Offscreen::create(std::shared_ptr<Compositor> compositor, int width, int height, size_t numberOfImages) {
    m_compositor(compositor);
    m_numberOfImages = std::min(numberOfImages, 2);
    m_pipelineDepth = m_numberOfImages - 1;

    spdlog::info("creating Offscreen renderer with {} images, pipeline depth of {}", m_numberOfImages, m_pipelineDepth);

    if (!m_framebuffer->create(width, height, m_numberOfImages)) {
        spdlog::error("Offscreen failed to create framebuffer of width: {}, height: {}, images: {}", width, height,
                numberOfImages);
        return false;
    }

    if (!m_renderSync->create(m_pipelineDepth)) {
        spdlog::error("Offscreen faild to create the render synchronization primitives.");
        return false;
    }

    // Prepare for readback by allocating GPU memory for readback images, checking for efficient copy operations from
    // the framebuffer to those readback images, and building command buffers to do the actual readback.
    m_readbackImages.reset(new ImageSet(m_device));
    if (!m_readbackImages->createHostTransferTarget(m_width, m_height, numberOfImages)) {
        spdlog::error("Window failed to create readback images.");
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

    while (!m_quit) {
        double deltaTime = 0.0;
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
            }
            deltaTime = m_deltaTime;
        }

        // Build list of active encoder frames we need to fill, if any.
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

        m_renderSync->waitForFrame(frameIndex);

        // There's two possibilities to consider. The first extreme is that the GPU is rendering frames much faster
        // than they can be consumed by either encoder or the window update. Let's say > 2x the window update rate.
        // The other extreme is that the GPU is taking much longer than the CPU. So either we're CPU bound or GPU bound.

        // The outputs of the rendering on an individual frame are always going to be copy operations. For encoding we
        // will want to copy the rendered frame to a GPU buffer that is accessible by the host, then that can go into a
        // queue for encoding. For window updates we will want to copy the rendered frame to the swapchain images with
        // low latency.

        // Let's consider the first use case, copyback operations for encoding. If the CPU is slow compared to the GPU
        // this will ultimately overwhelm memory as we'll have too many frames waiting for encode. There should be some
        // maximum outstanding number of frames waiting, and then the render thread should block until we're under that
        // limit. If the converse is true, the GPU is slow compared to the CPU, this doesn't seem to be an issue because
        // the encode thread will block until it has something to encode. SO - there needs to be a way to block the
        // render thread until there is a buffer available when the system has allocated the maximum number of
        // outstanding buffers.

        // For the purposes of Window updates what Window needs is low-latency access to a blit source. In the fast
        // GPU case this can mean that we won't blit every frame, far from it. But in the slow GPU case this could
        // mean either we blit the same frame multiple times, which is wasteful, or we don't update the swapchain
        // until there's an updated image (or a request for redraw, or some timeout). It seems as if both parties need
        // to be able to indicate new imagery is available. More specifically the window update thread may want a way
        // to wait until there's a fresh image, or some timeout, or an interrupt occurs asking for a repaint. And the
        // Offscreen, in the fast case, needs a way to understand that an additional blit is wasteful.
        // How about this - we keep two intermediate image buffers. When request blit is called we mark one of the

        // images as in use by the Window, and return it. We also tell the render loop to blit to the next one on the
        // next render frame. Every time Window requests a new image it gets the current one until the new one is marked
        // by the render frame as ready. So frames have 3 states - In Use, Update Requested, Ready. Because rendering
        // is pipelined there needs to be some general consideration of overall frame "readiness". I think we blit
        // before we re-render. The idea being that if the CPU is slow the GPU is pipelined well ahead of this and
        // we are waiting on frame availability for new renders. This means that the subsequent buffers are also ready
        // but as this is NRT we either need this blit for encoding and/or we need an updated framebuffer, but latency
        // is not as important to prioritize building a system that always gives the framebuffer the latest blit.
        // If the GPU is slow then the pipeline isn't full and this framebuffer is more recent? or even older?
    }
}

} // namespace vk

} // namespace scin
