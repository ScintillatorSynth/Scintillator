#include "vulkan/device.h"

#include "vulkan/instance.h"
#include "vulkan/window.h"

#include <iostream>
#include <set>
#include <vector>

namespace {
    const std::vector<const char*> device_extensions    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}

namespace scin {

namespace vk {

Device::Device(std::shared_ptr<Instance> instance) :
    instance_(instance),
    physical_device_(VK_NULL_HANDLE),
    allocator_(VK_NULL_HANDLE),
    graphics_family_index_(-1),
    present_family_index_(-1),
    device_(VK_NULL_HANDLE) {
}

Device::~Device() {
    if (device_ != VK_NULL_HANDLE) {
        Destroy();
    }
}

bool Device::FindPhysicalDevice(Window* window) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_->get(), &device_count, nullptr);
    if (device_count == 0) {
        std::cerr << "no Vulkan physical devices found." << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_->get(), &device_count, devices.data());
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);

        // Dedicated GPUs only for now. Device enumeration later.
        if (device_properties.deviceType !=
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            std::cout << "skipping indiscrete GPU" << std::endl;
            continue;
        }

        // Also needs to support graphics and present queue families.
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                queue_families.data());
        int family_index = 0;
        bool all_families_found = false;
        for (const auto& queue_family : queue_families) {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                    device,
                    family_index,
                    window->get_surface(),
                    &present_support);
            if (queue_family.queueCount == 0) {
                continue;
            }
            if (present_support) {
                present_family_index_ = family_index;
            }
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_family_index_ = family_index;
            }
            if (graphics_family_index_ >= 0 && present_family_index_ >= 0) {
                all_families_found = true;
                break;
            }

            ++family_index;
        }

        if (!all_families_found) {
            graphics_family_index_ = -1;
            present_family_index_ = -1;
            std::cout << "missing a queue family, skipping" << std::endl;
            continue;
        }

        // Check for supported device extensions.
        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr,
                &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(
                extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                available_extensions.data());
        std::set<std::string> required_extensions(device_extensions.begin(),
                device_extensions.end());
        for (const auto& extension : available_extensions) {
            required_extensions.erase(extension.extensionName);
        }

        if (!required_extensions.empty()) {
            std::cout << "some required extension missing, skipping"
                      << std::endl;
            for (const auto& extension : required_extensions) {
                std::cout << extension << std::endl;
            }
            continue;
        }

        // Check swap chain for suitability, it should support at least one
        // format and present mode.
        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, window->get_surface(),
                &format_count, nullptr);
        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, window->get_surface(),
                &present_mode_count, nullptr);

        if (format_count > 0 && present_mode_count > 0) {
            physical_device_ = device;
            break;
        }
    }

    if (physical_device_ == VK_NULL_HANDLE) {
        std::cerr << "no suitable physical device found." << std::endl;
        return false;
    }

    return true;
}

bool Device::Create(Window* window) {
    // FindPhysicalDevice() needs to be called first, if it hasn't been we call
    // it here.
    if (physical_device_ == VK_NULL_HANDLE) {
        if (!FindPhysicalDevice(window)) {
            return false;
        }
    }

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {
            static_cast<uint32_t>(graphics_family_index_),
            static_cast<uint32_t>(present_family_index_)
    };

    float queue_priority = 1.0;
    for (uint32_t queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family;
        create_info.queueCount = 1;
        create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(create_info);
    }

    VkPhysicalDeviceFeatures device_features = {};
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(
            queue_create_infos.size());
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(
            device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    if (vkCreateDevice(physical_device_, &device_create_info, nullptr,
            &device_) != VK_SUCCESS) {
        std::cerr << "failed to create logical device." << std::endl;
        return false;
    }

    vkGetDeviceQueue(device_, graphics_family_index_, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, present_family_index_, 0, &present_queue_);

    // Initialize memory allocator.
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = physical_device_;
    allocator_info.device = device_;
    vmaCreateAllocator(&allocator_info, &allocator_);

    return true;
}

void Device::Destroy() {
    vmaDestroyAllocator(allocator_);
    allocator_ = VK_NULL_HANDLE;
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
}

}    // namespace vk

}    // namespace scin

