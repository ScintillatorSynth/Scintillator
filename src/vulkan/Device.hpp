#ifndef SRC_VULKAN_DEVICE_HPP_
#define SRC_VULKAN_DEVICE_HPP_

#include "vulkan/Vulkan.hpp"

// Note this header also directly includes Vulkan, so should come after the Vulkan include, to allow GLFW first
// pass to include Vulkan and define macros as needed.
#include "vk_mem_alloc.h"

#include <memory>
#include <string>

namespace scin { namespace vk {

class DeviceInfo;
class Instance;
class Window;

// This class encapsulates both logical and physical devices, handing selection,
// creation, and destruction.
class Device {
public:
    Device(std::shared_ptr<Instance> instance, const DeviceInfo& deviceInfo);
    ~Device();

    // Try to find a suitable physical device, returns true if one exists.
    bool findPhysicalDevice(Window* window);

    // Creates the logical device, returns false on error.
    bool create(Window* window);
    void destroy();

    VkDevice get() { return m_device; }
    VkPhysicalDevice getPhysical() { return m_physicalDevice; }
    VmaAllocator allocator() { return m_allocator; }

    int graphicsFamilyIndex() const { return m_graphicsFamilyIndex; }
    int presentFamilyIndex() const { return m_presentFamilyIndex; }
    VkQueue graphicsQueue() { return m_graphicsQueue; }
    VkQueue presentQueue() { return m_presentQueue; }

private:
    std::shared_ptr<Instance> m_instance;
    VkPhysicalDevice m_physicalDevice;
    std::string m_name;
    int m_graphicsFamilyIndex;
    int m_presentFamilyIndex;
    VkDevice m_device;
    VmaAllocator m_allocator;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_DEVICE_HPP_
