#ifndef SRC_VULKAN_DEVICE_CHOOSER_HPP_
#define SRC_VULKAN_DEVICE_CHOOSER_HPP_

#include "vulkan/Vulkan.hpp"

#include <array>
#include <cstring>
#include <memory>
#include <vector>

namespace scin { namespace vk {

class Instance;

struct DeviceInfo {
    enum Type { kCPU, kDiscreteGPU, kIntegratedGPU, kOther };
    DeviceInfo(Type deviceType, std::string deviceName, const uint8_t* deviceUUID): type(deviceType), name(deviceName) {
        std::memcpy(uuid.data(), deviceUUID, sizeof(uuid));
    }

    Type type;
    std::string name;
    std::array<uint8_t, VK_UUID_SIZE> uuid;
};

/* Enumerates available Vulkan Devices, can pick the best suitable, locates SwiftShader.
 */
class DeviceChooser {
public:
    DeviceChooser(std::shared_ptr<Instance> instance);
    ~DeviceChooser();

    const std::vector<DeviceInfo>& enumerateAllDevices();

private:
    std::shared_ptr<Instance> m_instance;
    std::vector<DeviceInfo> m_devices;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_DEVICE_CHOOSER_HPP_
