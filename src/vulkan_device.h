#ifndef SRC_VULKAN_DEVICE_H_
#define SRC_VULKAN_DEVICE_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace scin {

class VulkanInstance;

// This class encapsulates both logical and physical devices, handing selection,
// creation, and destruction.
class VulkanDevice {
 public:
    VulkanDevice(VulkanInstance* instance);
    ~VulkanDevice();

    // Try to find a suitable physical device, returns true if one exists.
    bool FindPhysicalDevice(VkSurfaceKHR surface);
    // Creates the logical device, returns false on error.
    bool Create(VkSurfaceKHR surface);
    void Destroy();


 private:
    VulkanInstance* instance_;
    VkPhysicalDevice physical_device_;
    int graphics_family_index_;
    int present_family_index_;
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
};

}    // namespace scin

#endif    // SRC_VULKAN_DEVICE_H_
