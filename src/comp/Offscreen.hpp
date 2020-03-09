#ifndef SRC_COMP_OFFSCREEN_HPP_
#define SRC_COMP_OFFSCREEN_HPP_

#include "av/Encoder.hpp"

#include "vulkan/Vulkan.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace scin {

namespace av {
class BufferPool;
}

namespace vk {
class CommandBuffer;
class CommandPool;
class Device;
class Framebuffer;
class HostImage;
}

namespace comp {

class Canvas;
class Compositor;
class FrameTimer;
class RenderSync;
class Swapchain;

/*! Used in place of Window as a host for a Framebuffer when no Window is to be created.
 */
class Offscreen {
public:
    Offscreen(std::shared_ptr<vk::Device> device, int width, int height, int frameRate);
    ~Offscreen();

    /*! Offscreen will pipeline rendering to numberOfImages frames. Also starts the render thread, in a  paused state.
     * Additional pipelining costs GPU memory and will increase latency between frame render and encode or Window
     * update but may increase overall throughput.
     *
     * \param numberOfImages Should be at least 2.
     */
    bool create(size_t numberOfImages);

    /*! If this Offscreen is contained in a Window, we prepare an additional set of command buffers to blit from the
     * framebuffer images directly to the swapchain present images, with synchronization primitives provided by the
     * Window object. This allows the Window thread to schedule its own updates without requiring it to have its own
     * Vulkan queue, which some devices don't support.
     *
     * \param swapchain The swapchain object to use as a transfer target.
     * \param swapRenderSync The window-provided synchronization primitives to configure for render.
     * \return True on success, false on failure.
     */
    bool supportSwapchain(std::shared_ptr<Swapchain> swapchain, std::shared_ptr<RenderSync> swapRenderSync);

    /*! Start a thread to render at the provided framerate.
     */
    void runThreaded(std::shared_ptr<Compositor> compositor);

    /*! Render at the provided framerate on this thread.
     */
    void run(std::shared_ptr<Compositor> compositor);

    /*! Adds a video or image encoder to the list of encoders to call with readback images from subsequent frames.
     */
    void addEncoder(std::shared_ptr<scin::av::Encoder> encoder);

    /*! Request that the offscreen render thread blit recent render contents to the swapchain present image.
     *
     * \param swapchainImageIndex Which image in the swapchain to blit to.
     */
    void requestSwapchainBlit(uint32_t swapchainImageIndex);

    /*! Can be called before or after start(), will pause the rendering. Time can be advanced with advanceFrame()
     * calls if desired.
     */
    void pause();

    /*! For zero framerate, will render one frame, then advance time by dt and call the supplied callback.
     *
     * \param dt The amount of time to advance the frame by. Must be >= 0.
     * \param callback A function to call on completion of render, argument is the frame number.
     */
    void advanceFrame(double dt, std::function<void(size_t)> callback);

    /*! For stop the render thread and release all associated resources.
     */
    void stop();

    /*! Release any remaining resources associated with this Offscreen.
     */
    void destroy();

    std::shared_ptr<Canvas> canvas();
    int width() const { return m_width; }
    int height() const { return m_height; }

    /*! Returns true if the Offscreen is in snap shot mode (frameRate == 0).
     */
    bool isSnapShotMode() const { return m_snapShotMode; }

    std::shared_ptr<const FrameTimer> frameTimer() { return m_frameTimer; }

private:
    void threadMain(std::shared_ptr<Compositor> compositor);
    void processPendingEncodes(size_t frameIndex);
    bool writeCopyCommands(std::shared_ptr<vk::CommandBuffer> commandBuffer, size_t bufferIndex, VkImage sourceImage,
                           VkImage destinationImage);
    bool writeBlitCommands(std::shared_ptr<vk::CommandBuffer> commandBuffer, size_t bufferIndex, VkImage sourceImage,
                           VkImage destinationImage, VkImageLayout destinationLayout);
    bool blitAndPresent(size_t frameIndex, uint32_t swapImageIndex);

    std::shared_ptr<vk::Device> m_device;
    std::atomic<bool> m_quit;

    size_t m_numberOfImages;
    int m_width;
    int m_height;

    std::shared_ptr<FrameTimer> m_frameTimer;
    std::shared_ptr<vk::Framebuffer> m_framebuffer;
    std::unique_ptr<RenderSync> m_renderSync;
    std::shared_ptr<vk::CommandPool> m_commandPool;
    std::unique_ptr<scin::av::BufferPool> m_bufferPool;

    // Swapchain render support.
    std::shared_ptr<RenderSync> m_swapRenderSync;
    std::shared_ptr<Swapchain> m_swapchain;
    std::vector<std::shared_ptr<vk::CommandBuffer>> m_swapBlitCommands;

    // threadMain-only access
    std::thread m_renderThread;
    std::vector<std::shared_ptr<vk::CommandBuffer>> m_commandBuffers;
    std::vector<std::vector<scin::av::Encoder::SendBuffer>> m_pendingEncodes;
    std::vector<std::shared_ptr<vk::HostImage>> m_readbackImages;
    bool m_readbackSupportsBlit;
    // The index of this vector is the frameIndex, so the index of the pipelined framebuffer. The value is -1 if no
    // swapchain blit was requested, or the index of the swapchain source image (so 0 or 1).
    std::vector<int> m_pendingSwapchainBlits;

    std::mutex m_encodersMutex;
    std::list<std::shared_ptr<scin::av::Encoder>> m_encoders;
    std::shared_ptr<vk::CommandBuffer> m_readbackCommands;

    // Not mutex-protected read-only indicator if the frame rate is zero.
    bool m_snapShotMode;

    // Protects the render flag and the framerate, and deltaTime.
    std::mutex m_renderMutex;
    std::condition_variable m_renderCondition;
    bool m_render;
    bool m_swapBlitRequested;
    bool m_stagingRequested;
    uint32_t m_swapchainImageIndex;
    int m_frameRate;
    double m_deltaTime;
    std::function<void(size_t)> m_flushCallback;
};

} // namespace vk

} // namespace scin

#endif // SRC_COMP_OFFSCREEN_HPP_
