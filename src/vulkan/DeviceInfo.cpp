#include "vulkan/DeviceInfo.hpp"

#include "vulkan/Instance.hpp"

#include "spdlog/spdlog.h"

#include <cstring>
#include <set>

namespace {
const std::vector<const char*> windowDeviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
static const char* kMemoryBudgetExtension = "VK_EXT_memory_budget";
}

namespace scin { namespace vk {

DeviceInfo::DeviceInfo(std::shared_ptr<Instance> instance, VkPhysicalDevice device):
    m_instance(instance),
    m_physicalDevice(device),
    m_presentFamilyIndex(-1),
    m_graphicsFamilyIndex(-1),
    m_computeFamilyIndex(-1),
    m_numberOfMemoryHeaps(0),
    m_supportsWindow(false),
    m_supportsMemoryBudget(false) {}

bool DeviceInfo::build() {
    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);

    // Look for graphics and compute queue families, and ask about present family support.
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());
    int familyIndex = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Ask glfw for presentation support in this queue family.
        if (glfwGetPhysicalDevicePresentationSupport(m_instance->get(), m_physicalDevice, familyIndex) == GLFW_TRUE) {
            m_presentFamilyIndex = familyIndex;
        }
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_graphicsFamilyIndex = familyIndex;
        }
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            m_computeFamilyIndex = familyIndex;
        }
        if (m_graphicsFamilyIndex >= 0 && m_presentFamilyIndex >= 0 && m_computeFamilyIndex >= 0) {
            break;
        }

        ++familyIndex;
    }
    if (m_graphicsFamilyIndex == -1 || m_computeFamilyIndex == -1) {
        spdlog::warn("Device {} missing a graphics or compute queue family, not usable for graphics.", name());
        return false;
    }

    // Get some information about the memory properties of this device.
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);
    m_numberOfMemoryHeaps = memoryProperties.memoryHeapCount;

    // Base and windowing support requires some extensions, check for them.
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> optionalExtensions({ kMemoryBudgetExtension });
    std::set<std::string> windowExtensions(windowDeviceExtensions.begin(), windowDeviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        optionalExtensions.erase(extension.extensionName);
        windowExtensions.erase(extension.extensionName);
    }
    m_supportsMemoryBudget = optionalExtensions.empty();
    m_supportsWindow = windowExtensions.empty() && (m_presentFamilyIndex >= 0);

    vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_features);

    return true;
}

// static
const std::vector<const char*>& DeviceInfo::windowExtensions() { return windowDeviceExtensions; }
// static
const char* DeviceInfo::memoryBudgetExtension() { return kMemoryBudgetExtension; }

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
