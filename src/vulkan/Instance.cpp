#include "vulkan/Instance.hpp"

#include "spdlog/spdlog.h"

#include "Version.hpp"

#include <cstring>
#include <vector>

#if defined(SCIN_VALIDATE_VULKAN)
namespace {

VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
                                      const VkAllocationCallbacks* allocator,
                                      VkDebugUtilsMessengerEXT* debugMessenger) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        return func(instance, createInfo, allocator, debugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* allocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, allocator);
    }
}

const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (auto layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                    void* userData) {
    // The Severity bits can be combined, so we key log severity off of the most severe bit.
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        spdlog::error("Vulkan validation error: {}", callbackData->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        spdlog::warn("Vulkan validation warning: {}", callbackData->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        // Vulkan validation info is quite extensive, so we lower the spdlog level to debug to better match the use of
        // info/debug log levels in the rest of Scintillator.
        spdlog::debug("Vulkan validation info: {}", callbackData->pMessage);
    } else {
        spdlog::trace("Vulkan validation verbose: {}", callbackData->pMessage);
    }
    return VK_FALSE;
}

bool setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, debugMessenger) != VK_SUCCESS) {
        return false;
    }

    return true;
}

} // namespace

#endif // SCIN_VALIDATE_VULKAN

namespace scin { namespace vk {

Instance::Instance(): m_instance(VK_NULL_HANDLE) {}

Instance::~Instance() {
    if (m_instance != VK_NULL_HANDLE) {
        Destroy();
    }
}

bool Instance::create() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "scinsynth";
    appInfo.applicationVersion = VK_MAKE_VERSION(kScinVersionMajor, kScinVersionMinor, kScinVersionPatch);
    appInfo.pEngineName = "Scintillator";
    appInfo.engineVersion = VK_MAKE_VERSION(kScinVersionMajor, kScinVersionMinor, kScinVersionPatch);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if defined(SCIN_VALIDATE_VULKAN)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        spdlog::error("Failed to create Vulkan instance.");
        return false;
    }

#if defined(SCIN_VALIDATE_VULKAN)
    if (!setupDebugMessenger(m_instance, &m_debugMessenger)) {
        spdlog::error("Failed to create Vulkan Validation debug messenger.");
        return false;
    }
#endif

    return true;
}

void Instance::destroy() {
#if defined(SCIN_VALIDATE_VULKAN)
    destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
}

} // namespace vk

} // namespace scin
