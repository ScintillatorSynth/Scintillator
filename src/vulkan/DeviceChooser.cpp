#include "vulkan/DeviceChooser.hpp"

#include "vulkan/Instance.hpp"
#include "vulkan/Window.hpp"

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
    m_devices.reserve(deviceCount);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance->get(), &deviceCount, devices.data());
    for (const auto& device : devices) {
        m_devices.emplace_back(DeviceInfo(device));
        if (m_devices.back().type() < bestType) {
            m_bestDeviceIndex = m_devices.size() - 1;
            bestType = m_devices.back().type();
        }
    }
}

} // namespace vk

} // namespace scin
