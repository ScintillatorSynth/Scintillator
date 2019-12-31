#include "vulkan/Device.hpp"

#include "vulkan/Instance.hpp"
#include "vulkan/Window.hpp"

#include "spdlog/spdlog.h"

#include <set>
#include <vector>

namespace {
const std::vector<const char*> deviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

namespace scin { namespace vk {

Device::Device(std::shared_ptr<Instance> instance):
    m_instance(instance),
    m_physicalDevice(VK_NULL_HANDLE),
    m_allocator(VK_NULL_HANDLE),
    m_graphicsFamilyIndex_(-1),
    m_presentFamilyIndex_(-1),
    m_device(VK_NULL_HANDLE) {}

Device::~Device() {
    destroy();
}

bool Device::findPhysicalDevice(Window* window) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance->get(), &deviceCount, nullptr);
    if (deviceCount == 0) {
        spdlog::error("no Vulkan devices found.");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance->get(), &deviceCount, devices.data());
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Dedicated GPUs only for now. Device enumeration later.
        // if (deviceProperties.deviceType !=
        //    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        //    std::cout << "skipping indiscrete GPU" << std::endl;
        //    continue;
        //}

        // Also needs to support graphics and present queue families.
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        int familyIndex = 0;
        bool allFamiliesFound = false;
        for (const auto& queue_family : queueFamilies) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, window->getSurface(), &presentSupport);
            if (queue_family.queueCount == 0) {
                continue;
            }
            if (presentSupport) {
                m_presentFamilyIndex = familyIndex;
            }
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_graphicsFamilyIndex = familyIndex;
            }
            if (m_graphicsFamilyIndex >= 0 && m_presentFamilyIndex >= 0) {
                allFamiliesFound = true;
                break;
            }

            ++familyIndex;
        }

        if (!allFamiliesFound) {
            m_graphicsFamilyIndex = -1;
            m_presentFamilyIndex = -1;
            spdlog::info("missing a queue family, skipping");
            continue;
        }

        // Check for supported device extensions.
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        std::set<std::string> requiredExtensions(m_deviceextensions.begin(), m_deviceextensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        if (!requiredExtensions.empty()) {
            spdlog::info("some required extension missing, skipping");
            for (const auto& extension : requiredExtensions) {
                spdlog::info("{}", extension);
            }
            continue;
        }

        // Check swap chain for suitability, it should support at least one format and present mode.
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, window->getSurface(), &formatCount, nullptr);
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, window->getSurface(), &presentModeCount, nullptr);

        if (formatCount > 0 && presentModeCount > 0) {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        spdlog::error("no suitable Vulkan device found.");
        return false;
    }

    return true;
}

bool Device::create(Window* window) {
    // FindPhysicalDevice() needs to be called first, if it hasn't been we call it here.
    if (m_physicalDevice == VK_NULL_HANDLE) {
        if (!findPhysicalDevice(window)) {
            return false;
        }
    }

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
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceextensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = m_deviceextensions.data();

    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS) {
        std::cerr << "failed to create logical device." << std::endl;
        return false;
    }

    vkGetDeviceQueue(m_device, m_graphicsFamilyIndex, 0, &graphics_queue_);
    vkGetDeviceQueue(m_device, m_presentFamilyIndex, 0, &present_queue_);

    // Initialize memory allocator.
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    vmaCreateAllocator(&m_allocatorInfo, &m_allocator);

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
