#ifndef SRC_VULKAN_WINDOW_HPP_
#define SRC_VULKAN_WINDOW_HPP_

#include "vulkan/Vulkan.hpp"

#include <atomic>
#include <memory>
#include <vector>

namespace scin {

class Compositor;

namespace av {
class Encoder;
}

namespace vk {

class Canvas;
class CommandBuffer;
class Device;
class ImageSet;
class Instance;
class Offscreen;
class RenderSync;
class Swapchain;

/* While technically more a GLFW object than a Vulkan one, Window also maintains a VkSurfaceKHR handle, so lives with
 * the rest of the Vulkan objects.
 */
class Window {
public:
    Window(std::shared_ptr<Instance> instance, std::shared_ptr<Device> device, int width, int height, bool keepOnTop,
           int frameRate);
    ~Window();

    bool create();

    /*! Takes over the thead, will run until stop() is called or the Window is closed.
     *
     * \param compositor The root compositor to use for rendering.
     */
    void run(std::shared_ptr<Compositor> compositor);

    void destroy();

    /*! Typically called on another thread, will exit the run() loop on next iteration.
     */
    void stop() { m_stop = true; }

    GLFWwindow* get() { return m_window; }
    VkSurfaceKHR surface() { return m_surface; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    std::shared_ptr<Canvas> canvas();

private:
    void runDirectRendering(std::shared_ptr<Compositor> compositor);
    void runFixedFrameRate(std::shared_ptr<Compositor> compositor);

    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Device> m_device;
    int m_width;
    int m_height;
    bool m_keepOnTop;
    int m_frameRate;
    bool m_directRendering;
    GLFWwindow* m_window;
    VkSurfaceKHR m_surface;
    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<RenderSync> m_renderSync;

    // We keep the shared pointers to the command buffers until the frame is being re-rendered. This allows
    // the Compositor to change command buffers arbitrarily, and they won't get reclaimed by the system until
    // they are known finished rendering.
    std::shared_ptr<CommandBuffer> m_commandBuffers;
    std::atomic<bool> m_stop;

    // If in non realtime mode, we render to an offscreen framebuffer and blit the latest available image to the
    // swapchain images.
    std::shared_ptr<Offscreen> m_offscreen;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_WINDOW_HPP_
