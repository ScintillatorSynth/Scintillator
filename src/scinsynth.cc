#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>

const int WIDTH = 800;
const int HEIGHT = 600;

#if defined(SCIN_VALIDATE_VULKAN)
VkResult CreateDebugUtilsMessengerEXT(
  VkInstance instance,
  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(
  VkInstance instance,
  VkDebugUtilsMessengerEXT debugMessenger,
  const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
      func(instance, debugMessenger, pAllocator);
  }
}

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

bool CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
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

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

bool SetupDebugMessenger(VkInstance instance,
    VkDebugUtilsMessengerEXT* debugMessenger) {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = DebugCallback;

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
      debugMessenger) != VK_SUCCESS) {
      return false;
    }

  return true;
}
#endif  // SCIN_VALIDATE_VULKAN

int main() {
  // ========== glfw setup and window creation.
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
      "ScintillatorSynth", nullptr, nullptr);

  // ========== Vulkan setup.
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "scinsynth";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Scintillator";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  // Check for needed Vulkan extensions.
  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = &appInfo;

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions,
      glfwExtensions + glfwExtensionCount);

#if defined(SCIN_VALIDATE_VULKAN)
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(
      validationLayers.size());
  instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#else
  instanceCreateInfo.enabledLayerCount = 0;
#endif

  instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(
      extensions.size());
  instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

  // Create the Vulkan instance, our primary access to the Vulkan API.
  VkInstance instance;
  if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
    std::cerr << "failed to create instance." << std::endl;
    return EXIT_FAILURE;
  }

#if defined(SCIN_VALIDATE_VULKAN)
  VkDebugUtilsMessengerEXT debugMessenger;
  if (!SetupDebugMessenger(instance, &debugMessenger)) {
    std::cerr << "failed to create debug messenger" << std::endl;
    return EXIT_FAILURE;
  }
#endif

  // Create Vulkan physical device with needed queue families.
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    std::cerr << "no Vulkan physical devices found." << std::endl;
    return EXIT_FAILURE;
  }

  int graphicsFamilyIndex = -1;
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  for (const auto& device : devices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // Dedicated GPUs only for now. Device enumeration later.
    if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      continue;
    }

    // Also needs to support graphics queue families.
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
        nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
        queueFamilies.data());
    int familyIndex = 0;
    for (const auto& queueFamily : queueFamilies) {
      if (queueFamily.queueCount > 0 &&
          queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        graphicsFamilyIndex = familyIndex;
        break;
      }

      ++familyIndex;
    }

    if (graphicsFamilyIndex >= 0) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    std::cerr << "unable to find dedicated GPU." << std::endl;
    return EXIT_FAILURE;
  }

  // Create Vulkan logical device
  VkDevice device;
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures = {};
  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  // These fields are supposedly ignored in newer Vulkan implementations,
  // set anyway.
  deviceCreateInfo.enabledExtensionCount = 0;
#if defined(SCIN_VALIDATE_VULKAN)
  deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(
      validationLayers.size());
  deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#else
  deviceCreateInfo.enabledLayerCount = 0;
#endif

  // ========== Main loop.
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  // ========== Vulkan cleanup.
#if defined(SCIN_VALIDATE_VULKAN)
  DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
  vkDestroyInstance(instance, nullptr);

  // ========== glfw cleanup.
  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
