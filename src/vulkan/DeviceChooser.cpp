#include "vulkan/DeviceChooser.hpp"

#include "vulkan/Instance.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

DeviceChooser::DeviceChooser(std::shared_ptr<Instance> instance): m_instance(instance) {}

DeviceChooser::~DeviceChooser() {}

const std::vector<DeviceInfo>& DeviceChooser::enumerateAllDevices() {
    m_devices.clear();

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance->get(), &deviceCount, nullptr);
    if (deviceCount == 0) {
        spdlog::error("No Vulkan devices found.");
        return m_devices;
    }

    spdlog::error("Found {} Vulkan devices", deviceCount);

    m_devices.reserve(deviceCount);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance->get(), &deviceCount, devices.data());
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        DeviceInfo::Type deviceType;
        std::string deviceTypeName;
        switch (deviceProperties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                deviceType = DeviceInfo::Type::kIntegratedGPU;
                break;

            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                deviceType = DeviceInfo::Type::kDiscreteGPU;
                break;

            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                deviceType = DeviceInfo::Type::kCPU;
                break;

            default:
                deviceType = DeviceInfo::Type::kOther;
                break;
        }

        m_devices.emplace_back(DeviceInfo(deviceType, std::string(deviceProperties.deviceName),
                    deviceProperties.pipelineCacheUUID));
    }

    return m_devices;
}

} // namespace vk

} // namespace scin
