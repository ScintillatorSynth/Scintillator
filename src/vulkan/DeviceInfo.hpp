#ifndef SRC_VULKAN_DEVICE_INFO_
#define SRC_VULKAN_DEVICE_INFO_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <string>
#include <vector>

namespace scin { namespace vk {

class Instance;

/*! Wraps a VkPhysicalDevice and can answer basic questions about its properties and compatability with different use
 * cases, mainly in a Window or Surface rendering setup.
 */
class DeviceInfo {
public:
    /*! Type of devices. The number indicates relative performance (in general), with lower numbers being more
     * performant.
     */
    enum Type { kCPU = 2, kDiscreteGPU = 0, kIntegratedGPU = 1, kOther = 3, kNothing = 4 };

    DeviceInfo(std::shared_ptr<Instance> instance, VkPhysicalDevice device);
    static const std::vector<const char*>& windowExtensions();

    /*! Populate internal data structures with needed values. Will return false if the device can't support basic
     * graphics operations and shouldn't be considered a candidate device.
     */
    bool build();

    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    const char* name() const { return reinterpret_cast<const char*>(&m_properties.deviceName); }
    int presentFamilyIndex() const { return m_presentFamilyIndex; }
    int graphicsFamilyIndex() const { return m_graphicsFamilyIndex; }

    bool isSwiftShader() const;
    Type type() const;
    const char* typeName() const;
    const char* uuid() const { return m_uuid.data(); }
    bool supportsWindow() const { return m_supportsWindow; }

private:
    std::shared_ptr<Instance> m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkPhysicalDeviceProperties m_properties;
    std::string m_uuid;
    int m_presentFamilyIndex;
    int m_graphicsFamilyIndex;
    bool m_supportsWindow;
};

} // namespace vk
} // namespace scin

#endif // SRC_VULKAN_DEVICE_INFO_
