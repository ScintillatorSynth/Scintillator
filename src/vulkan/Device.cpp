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
    m_graphicsFamilyIndex(deviceInfo.graphicsFamilyIndex()),
    m_presentFamilyIndex(deviceInfo.presentFamilyIndex()),
    m_device(VK_NULL_HANDLE),
    m_allocator(VK_NULL_HANDLE),
    m_presentQueue(VK_NULL_HANDLE) {}

Device::~Device() { destroy(); }

bool Device::create(bool supportWindow, size_t numberOfGraphicsQueues) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { static_cast<uint32_t>(m_graphicsFamilyIndex) };
    if (supportWindow) {
        uniqueQueueFamilies.insert(static_cast<uint32_t>(m_presentFamilyIndex));
    }

    std::vector<float> queuePriorities(numberOfGraphicsQueues, 1.0);
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createInfo.queueFamilyIndex = queueFamily;
        if (queueFamily == static_cast<uint32_t>(m_graphicsFamilyIndex)) {
            createInfo.queueCount = numberOfGraphicsQueues;
        } else {
            createInfo.queueCount = 1;
        }
        createInfo.pQueuePriorities = queuePriorities.data();
        queueCreateInfos.push_back(createInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    if (supportWindow) {
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceInfo::windowExtensions().size());
        deviceCreateInfo.ppEnabledExtensionNames = DeviceInfo::windowExtensions().data();
    }

    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS) {
        spdlog::error("Device {} failed to create logical device.", m_name);
        return false;
    }

    m_graphicsQueues.resize(numberOfGraphicsQueues);
    for (auto i = 0; i < numberOfGraphicsQueues; ++i) {
        vkGetDeviceQueue(m_device, m_graphicsFamilyIndex, i, &m_graphicsQueues[i]);
    }

    if (supportWindow) {
        vkGetDeviceQueue(m_device, m_presentFamilyIndex, 0, &m_presentQueue);
    }

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
