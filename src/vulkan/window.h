#ifndef SRC_VULKAN_WINDOW_H_
#define SRC_VULKAN_WINDOW_H_

#include "vulkan/scin_include_vulkan.h"

#include <memory>

namespace scin {

namespace vk {

class CommandPool;
class Device;
class Instance;
class Swapchain;

// While technically more a GLFW object than a Vulkan one, Window also maintains
// a VkSurfaceKHR handle, so lives with the rest of the Vulkan objects.
class Window {
  public:
    Window(std::shared_ptr<Instance> instance);
    ~Window();

    bool Create(int width, int height);
    bool CreateSemaphores(Device* device);
    void Run(Device* device, Swapchain* swapchain, CommandPool* command_pool);
    void DestroySemaphores(Device* device);
    void Destroy();

    GLFWwindow* get() { return window_; }
    VkSurfaceKHR get_surface() { return surface_; }
    int width() const { return width_; }
    int height() const { return height_; }

  private:
    std::shared_ptr<Instance> instance_;
    int width_;
    int height_;
    GLFWwindow* window_;
    VkSurfaceKHR surface_;
    VkSemaphore image_available_semaphore_;
    VkSemaphore render_finished_semaphore_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_WINDOW_H_

