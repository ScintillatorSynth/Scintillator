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
    m_numberOfMemoryHeaps(deviceInfo.numberOfMemoryHeaps()),
    m_supportsMemoryBudget(deviceInfo.supportsMemoryBudget()),
    m_supportsSamplerAnisotropy(deviceInfo.supportsSamplerAnisotropy()),
    m_device(VK_NULL_HANDLE),
    m_allocator(VK_NULL_HANDLE),
    m_presentQueue(VK_NULL_HANDLE) {}

Device::~Device() { destroy(); }

bool Device::create(bool supportWindow) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { static_cast<uint32_t>(m_graphicsFamilyIndex) };
    if (supportWindow) {
        uniqueQueueFamilies.insert(static_cast<uint32_t>(m_presentFamilyIndex));
    }

    spdlog::info("Device {} has {} unique queue families.", m_name, uniqueQueueFamilies.size());

    float queuePriority = 1.0;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createInfo.queueFamilyIndex = queueFamily;
        createInfo.queueCount = 1;
        createInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(createInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    if (m_supportsSamplerAnisotropy) {
        deviceFeatures.samplerAnisotropy = VK_TRUE;
    }

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    std::vector<const char*> extensions;
    if (supportWindow) {
        extensions.insert(extensions.begin(), DeviceInfo::windowExtensions().begin(),
                          DeviceInfo::windowExtensions().end());
    }
    if (m_supportsMemoryBudget) {
        extensions.push_back(DeviceInfo::memoryBudgetExtension());
    }
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS) {
        spdlog::error("Device {} failed to create logical device.", m_name);
        return false;
    }

    vkGetDeviceQueue(m_device, m_graphicsFamilyIndex, 0, &m_graphicsQueue);
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

bool Device::getGraphicsMemoryBudget(size_t& bytesUsedOut, size_t& bytesBudgetOut) {
    bytesUsedOut = 0;
    bytesBudgetOut = 0;

    std::vector<VmaBudget> budgets(m_numberOfMemoryHeaps);
    vmaGetBudget(m_allocator, budgets.data());
    if (m_supportsMemoryBudget) {
        for (const auto& budget : budgets) {
            bytesUsedOut += budget.usage;
            bytesBudgetOut += budget.budget;
        }
    } else {
        // Some devices don't support VK_EXT_memory_budget extension, and so won't provide budget
        // data. We approximate memory usage with block bytes allocated, which doesn't count for
        // some Vulkan allocations that don't use the allocator (like Pipelines and Swapchains),
        // and we won't have the budget limit available.
        for (const auto& budget : budgets) {
            bytesUsedOut += budget.blockBytes;
        }
    }

    return true;
}


} // namespace vk

} // namespace scin
