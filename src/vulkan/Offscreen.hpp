#ifndef SRC_VULKAN_OFFSCREEN_HPP_
#define SRC_VULKAN_OFFSCREEN_HPP_

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

class Compositor;

namespace av {
class BufferPool;
}

namespace vk {

class Canvas;
class CommandBuffer;
class CommandPool;
class Device;
class Framebuffer;
class ImageSet;
class RenderSync;
class Swapchain;

/*! Used in place of Window as a host for a Framebuffer when no Window is to be created.
 */
class Offscreen {
public:
    Offscreen(std::shared_ptr<Device> device);
    ~Offscreen();

    /*! Offscreen will pipeline rendering to numberOfImages frames. Also starts the render thread, in a  paused state.
     * Additional pipelining costs GPU memory and will increase latency between frame render and encode or Window
     * update but may increase overall throughput.
     *
     * \param numberOfImages Should be at least 2.
     */
    bool create(int width, int height, size_t numberOfImages);

    /*! If this Offscreen is contained in a Window, we create additional copy targets for the offscreen to allow the
     * Window to update independently. This also creates the command buffers for blitting between all possible
     * combinations of image source and swapchain image destination.
     *
     * \param swapchain The swapchain object to use as a transfer target.
     * \return True on success, false on failure.
     */
    bool createSwapchainSources(Swapchain* swapchain);

    /*! Start a thread to render at the provided framerate.
     */
    void runThreaded(std::shared_ptr<Compositor> compositor, int frameRate);

    /*! Render at the provided framerate on this thread.
     */
    void run(std::shared_ptr<Compositor> compositor, int frameRate);

    /*! Adds a video or image encoder to the list of encoders to call with readback images from subsequent frames.
     */
    void addEncoder(std::shared_ptr<scin::av::Encoder> encoder);

    /*! Returns a command buffer that will blit from the most recently updated copy of the framebuffer.
     *
     * \return A command buffer useable in a swapchain update, or nullptr if no buffer has been prepared.
     */
    std::shared_ptr<CommandBuffer> getSwapchainBlit();

    /*! Can be called before or after start(), will pause the rendering. Time can be advanced with advanceFrame()
     * calls if desired.
     */
    void pause();

    /*! For nonzero framerate, when paused, will render one additional frame.
     */
    void advanceFrame();

    /*! For zero framerate, will advance time by dt and then render one additional frame.
     *
     * \param dt The amount of time to advance the frame by. Must be >= 0.
     */
    void renderFrame(double dt);

    /*! For stop the render thread and release all associated resources.
     */
    void stop();

    /*! Release any remaining resources associated with this Offscreen.
     */
    void destroy();

    std::shared_ptr<Canvas> canvas();

private:
    void threadMain(std::shared_ptr<Compositor> compositor);
    void processPendingBlits(size_t frameIndex);
    bool writeCopyCommands(std::shared_ptr<CommandBuffer> commandBuffer, size_t bufferIndex, int width, int height,
                           VkImage sourceImage, VkImage destinationImage);
    bool writeBlitCommands(std::shared_ptr<CommandBuffer> commandBuffer, size_t bufferIndex, int width, int height,
                           VkImage sourceImage, VkImage destinationImage);

    std::shared_ptr<Device> m_device;
    std::atomic<bool> m_quit;

    size_t m_numberOfImages;

    std::shared_ptr<Framebuffer> m_framebuffer;
    std::unique_ptr<RenderSync> m_renderSync;
    std::shared_ptr<CommandPool> m_commandPool;
    std::unique_ptr<scin::av::BufferPool> m_bufferPool;

    // Swapchain render support.
    std::unique_ptr<ImageSet> m_swapSources;
    std::vector<std::shared_ptr<CommandBuffer>> m_sourceBlitCommands;
    std::vector<std::shared_ptr<CommandBuffer>> m_swapBlitCommands;

    std::mutex m_swapMutex;
    enum SourceImageState { kEmpty, kRequested, kPipelined, kReady, kInUse };
    std::array<SourceImageState, 2> m_sourceStates;

    // threadMain-only access
    std::thread m_renderThread;
    std::vector<std::shared_ptr<CommandBuffer>> m_commandBuffers;
    std::vector<std::vector<scin::av::Encoder::SendBuffer>> m_pendingEncodes;
    std::shared_ptr<ImageSet> m_readbackImages;
    bool m_readbackSupportsBlit;
    // The index of this vector is the frameIndex, so the index of the pipelined framebuffer. The value is -1 if no
    // swapchain blit was requested, or the index of the swapchain source image (so 0 or 1).
    std::vector<int> m_pendingSwapchainBlits;

    std::mutex m_encodersMutex;
    std::list<std::shared_ptr<scin::av::Encoder>> m_encoders;
    std::shared_ptr<CommandBuffer> m_readbackCommands;

    // Protects the render flag and the framerate, and deltaTime.
    std::mutex m_renderMutex;
    std::condition_variable m_renderCondition;
    bool m_render;
    int m_frameRate;
    double m_deltaTime;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_OFFSCREEN_HPP_
