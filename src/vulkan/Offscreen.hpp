#ifndef SRC_VULKAN_OFFSCREEN_HPP_
#define SRC_VULKAN_OFFSCREEN_HPP_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace scin { namespace vk {

class Compositor;
class Device;
class Framebuffer;
class RenderSync;

/*! Used in place of Window as a host for a Framebuffer when no Window is to be created.
 */
class Offscreen {
public:
    Offscreen(std::shared_ptr<Device> device);
    ~Offscreen();

    /*! Offscreen will pipeline rendering to numberOfImages - 1 frames, keeping one image available for
     * readback/encoding as well as rendering to any window that may be present. Also starts the render thread, in a
     * paused state.
     *
     * \param numberOfImages Should be at least 2.
     */
    bool create(std::shared_ptr<Compositor> compositor, int width, int height, size_t numberOfImages);

    /*! Start a thread to render at the provided framerate.
     */
    void start(int frameRate);

    /*! Adds a video or image encoder to the list of encoders to call with readback images from subsequent frames.
     */
    void addEncoder(std::shared_ptr<scin::av::Encoder> encoder);

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

private:
    void threadMain();

    std::shared_ptr<Device> m_device;
    std::atomic<bool> m_quit;

    size_t m_numberOfImages;
    size_t m_pipelineDepth;

    std::shared_ptr<Framebuffer> m_framebuffer;
    std::unique_ptr<RenderSync> m_renderSync;
    std::unique_ptr<CommandPool> m_commandPool;
    std::shared_ptr<Compositor> m_compositor;

    std::shared_ptr<ImageSet> m_readbackImages;
    bool m_readbackSupportsBlit;
    std::mutex m_encodersMutex;
    std::list<std::shared_ptr<scin::av::Encoder>> m_encoders;
    std::shared_ptr<CommandBuffer> m_readbackCommands;

    std::thread m_renderThread;

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
