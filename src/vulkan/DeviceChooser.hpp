#ifndef SRC_VULKAN_DEVICE_CHOOSER_HPP_
#define SRC_VULKAN_DEVICE_CHOOSER_HPP_

#include "vulkan/DeviceInfo.hpp"

#include <array>
#include <memory>
#include <vector>

namespace scin { namespace vk {

class Instance;
class Device;
class Window;

/* Enumerates available Vulkan Devices and populates an internal list of DeviceInfo objects.
 */
class DeviceChooser {
public:
    DeviceChooser(std::shared_ptr<Instance> instance);
    ~DeviceChooser();

    /*! Query Vulkan and build the internal list of DeviceInfo structs.
     */
    void enumerateAllDevices();

    const std::vector<DeviceInfo>& devices() const { return m_devices; }

    /*! Returns the index of the highest performance device, or zero if there are no devices detected.
     */
    size_t bestDeviceIndex() const { return m_bestDeviceIndex; }

private:
    std::shared_ptr<Instance> m_instance;
    std::vector<DeviceInfo> m_devices;
    size_t m_bestDeviceIndex;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_DEVICE_CHOOSER_HPP_
