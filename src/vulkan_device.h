#ifndef SRC_VULKAN_DEVICE_H_
#define SRC_VULKAN_DEVICE_H_

#include "scin_include_vulkan.h"

#include <memory>

namespace scin {

class VulkanInstance;
class VulkanWindow;

// This class encapsulates both logical and physical devices, handing selection,
// creation, and destruction.
class VulkanDevice {
  public:
    VulkanDevice(std::shared_ptr<VulkanInstance> instance);
    ~VulkanDevice();

    // Try to find a suitable physical device, returns true if one exists.
    bool FindPhysicalDevice(VulkanWindow* window);
    // Creates the logical device, returns false on error.
    bool Create(VulkanWindow* window);
    void Destroy();

    VkDevice get() { return device_; }
    VkPhysicalDevice get_physical() { return physical_device_; }

    int graphics_family_index() const { return graphics_family_index_; }
    int present_family_index() const { return present_family_index_; }

  private:
    std::shared_ptr<VulkanInstance> instance_;
    VkPhysicalDevice physical_device_;
    int graphics_family_index_;
    int present_family_index_;
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
};

}    // namespace scin

#endif    // SRC_VULKAN_DEVICE_H_

