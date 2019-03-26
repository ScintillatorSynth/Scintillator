#ifndef SRC_VULKAN_WINDOW_H_
#define SRC_VULKAN_WINDOW_H_

#include "scin_include_vulkan.h"

#include <memory>

namespace scin {

class VulkanInstance;

class VulkanWindow {
  public:
    VulkanWindow(std::shared_ptr<VulkanInstance> instance);
    ~VulkanWindow();

    bool Create(int width, int height);
    void Run();
    void Destroy();

    GLFWwindow* get() { return window_; }
    VkSurfaceKHR get_surface() { return surface_; }

  private:
    std::shared_ptr<VulkanInstance> instance_;
    GLFWwindow* window_;
    VkSurfaceKHR surface_;
};

}  // namespace scin

#endif  // SRC_VULKAN_WINDOW_H_

