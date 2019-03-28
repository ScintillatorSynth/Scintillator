#include "scin_include_vulkan.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "vulkan_swap_chain.h"
#include "vulkan_window.h"

#include <memory>

int main() {
    glfwInit();

    // ========== Vulkan setup.
    std::shared_ptr<scin::VulkanInstance> vk_instance(
            new scin::VulkanInstance());
    if (!vk_instance->Create()) {
        return EXIT_FAILURE;
    }

    scin::VulkanWindow window(vk_instance);
    if (!window.Create(800, 600)) {
        return EXIT_FAILURE;
    }

    // Create Vulkan physical and logical device.
    std::shared_ptr<scin::VulkanDevice> vk_device(
            new scin::VulkanDevice(vk_instance));
    if (!vk_device->Create(&window)) {
        return EXIT_FAILURE;
    }

    // Configure swap chain based on device and surface capabilities.
    scin::VulkanSwapChain vk_swap_chain(vk_device);
    if (!vk_swap_chain.Create(&window)) {
        return EXIT_FAILURE;
    }

    // ========== Main loop.
    window.Run();

    // ========== Vulkan cleanup.
    vk_swap_chain.Destroy();
    vk_device->Destroy();
    vk_instance->Destroy();

    // ========== glfw cleanup.
    glfwTerminate();

    return EXIT_SUCCESS;
}

