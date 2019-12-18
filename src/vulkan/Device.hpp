#ifndef SRC_VULKAN_DEVICE_HPP_
#define SRC_VULKAN_DEVICE_HPP_

#include "vulkan/Vulkan.hpp"

// Note this header also directly includes Vulkan, so should come after the Vulkan include, to allow GLFW first
// pass to include Vulkan and define macros as needed.
#include "vk_mem_alloc.h"

#include <memory>

namespace scin { namespace vk {

class Instance;
class Window;

// This class encapsulates both logical and physical devices, handing selection,
// creation, and destruction.
class Device {
public:
    Device(std::shared_ptr<Instance> instance);
    ~Device();

    // Try to find a suitable physical device, returns true if one exists.
    bool FindPhysicalDevice(Window* window);

    // Creates the logical device, returns false on error.
    bool Create(Window* window);
    void Destroy();

    VkDevice get() { return device_; }
    VkPhysicalDevice get_physical() { return physical_device_; }
    VmaAllocator allocator() { return allocator_; }

    int graphics_family_index() const { return graphics_family_index_; }
    int present_family_index() const { return present_family_index_; }
    VkQueue graphics_queue() { return graphics_queue_; }
    VkQueue present_queue() { return present_queue_; }

private:
    std::shared_ptr<Instance> instance_;
    VkPhysicalDevice physical_device_;
    int graphics_family_index_;
    int present_family_index_;
    VkDevice device_;
    VmaAllocator allocator_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_DEVICE_HPP_
