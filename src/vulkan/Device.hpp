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

    /*! Creates the logical device, optionally supporting windowed presentation.
     *
     * \param supportWindow If true, configure the device to support presentation of framebuffers via swapchain.
     * \return true on success, false on error.
     */
    bool create(bool supportWindow);

    void destroy();

    /*! Query the graphics memory allocator for current memory budget information. Uses a cheaper query but the
     * information is approximate.
     *
     * \param bytesUsedOut The number of graphics memory bytes consumed by scintillator.
     * \param bytesBudgetOut The maximum amount of graphics memory that scintillator can safely consume.
     * \return true on success, false on query falure.
     */
    bool getGraphicsMemoryBudget(size_t& bytesUsedOut, size_t& bytesBudgetOut) const;

    VkDevice get() { return m_device; }
    VkPhysicalDevice physical() { return m_physicalDevice; }
    VmaAllocator allocator() { return m_allocator; }

    int graphicsFamilyIndex() const { return m_graphicsFamilyIndex; }
    int presentFamilyIndex() const { return m_presentFamilyIndex; }
    int computeFamilyIndex() const { return m_computeFamilyIndex; }
    VkQueue computeQueue() { return m_computeQueue; }
    VkQueue graphicsQueue() { return m_graphicsQueue; }
    VkQueue presentQueue() { return m_presentQueue; }
    bool supportsSamplerAnisotropy() const { return m_supportsSamplerAnisotropy; }
    uint32_t maxImageDimension2D() const { return m_maxImageDimension2D; }

private:
    std::shared_ptr<Instance> m_instance;
    VkPhysicalDevice m_physicalDevice;
    std::string m_name;
    int m_computeFamilyIndex;
    int m_graphicsFamilyIndex;
    int m_presentFamilyIndex;
    int m_numberOfMemoryHeaps;
    bool m_supportsMemoryBudget;
    bool m_supportsSamplerAnisotropy;
    uint32_t m_maxImageDimension2D;
    VkDevice m_device;
    VmaAllocator m_allocator;
    VkQueue m_computeQueue;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
};

} // namespace vk
} // namespace scin

#endif // SRC_VULKAN_DEVICE_HPP_
