#include "scin_include_vulkan.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
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
    scin::VulkanDevice vk_device(vk_instance);
    if (!vk_device.Create(window.get_surface())) {
        return EXIT_FAILURE;
    }

    // ========== Main loop.
    window.Run();

    // ========== Vulkan cleanup.
    vk_device.Destroy();

    vk_instance->Destroy();

    // ========== glfw cleanup.
    glfwTerminate();

    return EXIT_SUCCESS;
}

