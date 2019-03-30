#include "vulkan/device.h"
#include "vulkan/instance.h"
#include "vulkan/scin_include_vulkan.h"
#include "vulkan/swap_chain.h"
#include "vulkan/window.h"

#include <memory>

int main() {
    glfwInit();

    // ========== Vulkan setup.
    std::shared_ptr<scin::vk::Instance> instance(new scin::vk::Instance());
    if (!instance->Create()) {
        return EXIT_FAILURE;
    }

    scin::vk::Window window(instance);
    if (!window.Create(800, 600)) {
        return EXIT_FAILURE;
    }

    // Create Vulkan physical and logical device.
    std::shared_ptr<scin::vk::Device> device(new scin::vk::Device(instance));
    if (!device->Create(&window)) {
        return EXIT_FAILURE;
    }

    // Configure swap chain based on device and surface capabilities.
    scin::vk::SwapChain swap_chain(device);
    if (!swap_chain.Create(&window)) {
        return EXIT_FAILURE;
    }

    // ========== Main loop.
    window.Run();

    // ========== Vulkan cleanup.
    swap_chain.Destroy();
    device->Destroy();
    instance->Destroy();

    // ========== glfw cleanup.
    glfwTerminate();

    return EXIT_SUCCESS;
}

