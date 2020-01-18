#include "vulkan/DeviceInfo.hpp"

#include "vulkan/Window.hpp"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

#include <cstring>
#include <set>

namespace {
const std::vector<const char*> windowDeviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

namespace scin { namespace vk {

DeviceInfo::DeviceInfo(VkPhysicalDevice device):
    m_physicalDevice(device),
    m_presentFamilyIndex(-1),
    m_graphicsFamilyIndex(-1) {
    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);
    for (auto i = 0; i < VK_UUID_SIZE; ++i) {
        m_uuid += fmt::format("{:x}", m_properties.pipelineCacheUUID[i]);
    }
}

// static
const std::vector<const char*>& DeviceInfo::windowExtensions() { return windowDeviceExtensions; }

bool DeviceInfo::supportsWindow(Window* window) {
    // Check both graphics and present queue families.
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());
    int familyIndex = 0;
    bool allFamiliesFound = false;
    for (const auto& queue_family : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, familyIndex, window->getSurface(), &presentSupport);
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
        spdlog::warn("Device {} missing a queue family, not compatible with window", name());
        return false;
    }

    // Check for supported device extensions.
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> requiredExtensions(windowDeviceExtensions.begin(), windowDeviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    if (!requiredExtensions.empty()) {
        spdlog::warn("Device {} missing some required extensions:", name());
        for (const auto& extension : requiredExtensions) {
            spdlog::warn("  {}", extension);
        }
        return false;
    }

    // Check swap chain for suitability, it should support at least one format and present mode.
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, window->getSurface(), &formatCount, nullptr);
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, window->getSurface(), &presentModeCount, nullptr);
    if (formatCount == 0 || presentModeCount == 0) {
        spdlog::warn("Device {} missing either a format or present mode.", name());
        return false;
    }

    return true;
}

bool DeviceInfo::isSwiftShader() const { return std::strncmp(name(), "SwiftShader", 11) == 0; }

DeviceInfo::Type DeviceInfo::type() const {
    switch (m_properties.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return DeviceInfo::Type::kIntegratedGPU;

    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return DeviceInfo::Type::kDiscreteGPU;

    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return DeviceInfo::Type::kCPU;

    default:
        return DeviceInfo::Type::kOther;
    }
}

const char* DeviceInfo::typeName() const {
    switch (m_properties.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "Integrated GPU";

    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "Discrete GPU";

    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "CPU";

    default:
        return "Other";
    }
}

} // namespace vk

} // namespace scin
