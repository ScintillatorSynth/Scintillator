#ifndef SRC_VULKAN_WINDOW_HPP_
#define SRC_VULKAN_WINDOW_HPP_

#include "vulkan/Vulkan.hpp"

#include <atomic>
#include <memory>
#include <vector>

namespace scin {

class Compositor;

namespace vk {

class Canvas;
class CommandBuffer;
class Device;
class Instance;
class Swapchain;

/* While technically more a GLFW object than a Vulkan one, Window also maintains a VkSurfaceKHR handle, so lives with
 * the rest of the Vulkan objects.
 */
class Window {
public:
    Window(std::shared_ptr<Instance> instance);
    ~Window();

    // TODO: weird dependency ordering here where the surface needs to be created in this function, and the device
    // needs to know the surface in order to pick a suitable device that supports rendering to that surface. If we
    // aren't rendering to an onscreen window a surface will have to be provided to Device some other way, and perhaps
    // Device will have a different create() function for that context.
    bool create(int width, int height);
    bool createSwapchain(std::shared_ptr<Device> device);
    bool createSyncObjects();
    void run(std::shared_ptr<Compositor> compositor);
    void destroySyncObjects();
    void destroySwapchain();
    void destroy();

    /*! Typically called on another thread, will exit the run() loop on next iteration.
     */
    void stop() { m_stop = true; }

    GLFWwindow* get() { return m_window; }
    VkSurfaceKHR getSurface() { return m_surface; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    std::shared_ptr<Canvas> canvas();

private:
    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Device> m_device;
    int m_width;
    int m_height;
    GLFWwindow* m_window;
    VkSurfaceKHR m_surface;
    std::shared_ptr<Swapchain> m_swapchain;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    // We keep the shared pointers to the command buffers until the frame is being re-rendered. This allows
    // the Compositor to change command buffers arbitrarily, and they won't get reclaimed by the system until
    // they are known finished rendering.
    std::vector<std::vector<std::shared_ptr<CommandBuffer>>> m_commandBuffers;
    std::atomic<bool> m_stop;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_WINDOW_HPP_
