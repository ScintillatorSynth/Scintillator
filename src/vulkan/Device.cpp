#include "vulkan/Device.hpp"

#include "vulkan/DeviceInfo.hpp"
#include "vulkan/Instance.hpp"
#include "vulkan/Window.hpp"

#include "spdlog/spdlog.h"

#include <vector>
#include <set>

namespace scin { namespace vk {

Device::Device(std::shared_ptr<Instance> instance, const DeviceInfo& deviceInfo):
    m_instance(instance),
    m_physicalDevice(deviceInfo.physicalDevice()),
    m_name(deviceInfo.name()),
    m_allocator(VK_NULL_HANDLE),
    m_graphicsFamilyIndex(deviceInfo.graphicsFamilyIndex()),
    m_presentFamilyIndex(deviceInfo.presentFamilyIndex()),
    m_device(VK_NULL_HANDLE) {}

Device::~Device() { destroy(); }

bool Device::create(Window* window) {
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queueFamilies = { static_cast<uint32_t>(m_graphicsFamilyIndex),
                                                static_cast<uint32_t>(m_presentFamilyIndex) };

    float queue_priority = 1.0;
    for (uint32_t queue_family : unique_queueFamilies) {
        VkDeviceQueueCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family;
        create_info.queueCount = 1;
        create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(create_info);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queue_create_infos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceInfo::windowExtensions().size());
    deviceCreateInfo.ppEnabledExtensionNames = DeviceInfo::windowExtensions().data();

    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS) {
        spdlog::error("Device {} failed to create logical device.", m_name);
        return false;
    }

    vkGetDeviceQueue(m_device, m_graphicsFamilyIndex, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentFamilyIndex, 0, &m_presentQueue);

    // Initialize memory allocator.
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    vmaCreateAllocator(&allocatorInfo, &m_allocator);

    return true;
}

void Device::destroy() {
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
}

} // namespace vk

} // namespace scin
