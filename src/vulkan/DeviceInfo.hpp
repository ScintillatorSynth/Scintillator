#ifndef SRC_VULKAN_DEVICE_INFO_
#define SRC_VULKAN_DEVICE_INFO_

#include "vulkan/Vulkan.hpp"

#include <string>
#include <vector>

namespace scin { namespace vk {

class Window;

/*! Wraps a VkPhysicalDevice and can answer basic questions about its properties and compatability with different use
 * cases, mainly in a Window or Surface rendering setup.
 */
class DeviceInfo {
public:
    /*! Type of devices. The number indicates relative performance (in general), with lower numbers being more
     * performant.
     */
    enum Type { kCPU = 2, kDiscreteGPU = 0, kIntegratedGPU = 1, kOther = 3, kNothing = 4 };

    DeviceInfo(VkPhysicalDevice device);
    static const std::vector<const char*>& windowExtensions();

    // TODO: this right now has to be called before the device can be constructed, to get the queuefamily indices.
    // Weird side effect for a query method.
    bool supportsWindow(Window* window);

    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    const char* name() const { return reinterpret_cast<const char*>(&m_properties.deviceName); }
    int presentFamilyIndex() const { return m_presentFamilyIndex; }
    int graphicsFamilyIndex() const { return m_graphicsFamilyIndex; }

    bool isSwiftShader() const;
    Type type() const;
    const char* typeName() const;
    const char* uuid() const { return m_uuid.data(); }

private:
    VkPhysicalDevice m_physicalDevice;
    VkPhysicalDeviceProperties m_properties;
    std::string m_uuid;
    int m_presentFamilyIndex;
    int m_graphicsFamilyIndex;
};

} // namespace vk
} // namespace scin

#endif // SRC_VULKAN_DEVICE_INFO_

