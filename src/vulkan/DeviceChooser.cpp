#include "vulkan/DeviceChooser.hpp"

#include "vulkan/Instance.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

DeviceChooser::DeviceChooser(std::shared_ptr<Instance> instance): m_instance(instance), m_bestDeviceIndex(-1) {}

DeviceChooser::~DeviceChooser() {}

void DeviceChooser::enumerateAllDevices() {
    m_devices.clear();

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance->get(), &deviceCount, nullptr);
    if (deviceCount == 0) {
        spdlog::error("no Vulkan devices found.");
        return;
    }

    DeviceInfo::Type bestType = DeviceInfo::Type::kNothing;
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance->get(), &deviceCount, devices.data());
    for (const auto& device : devices) {
        DeviceInfo deviceInfo(m_instance, device);
        if (deviceInfo.build()) {
            m_devices.push_back(deviceInfo);
            if (deviceInfo.type() < bestType) {
                m_bestDeviceIndex = m_devices.size() - 1;
                bestType = deviceInfo.type();
            }
        }
    }
}

} // namespace vk

} // namespace scin
