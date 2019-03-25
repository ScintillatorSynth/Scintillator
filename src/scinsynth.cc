#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan_device.h"
#include "vulkan_instance.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>

const int WIDTH = 800;
const int HEIGHT = 600;


int main() {
  // ========== glfw setup and window creation.
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
      "ScintillatorSynth", nullptr, nullptr);

  // ========== Vulkan setup.
  scin::VulkanInstance vk_instance;
  if (!vk_instance.Create()) {
    return EXIT_FAILURE;
  }

  // Create Vulkan window surface.
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(vk_instance.get(), window, nullptr, &surface)
      != VK_SUCCESS) {
    std::cerr << "failed to create surface" << std::endl;
    return EXIT_FAILURE;
  }

  // Create Vulkan logical device.
  scin::VulkanDevice vk_device(&vk_instance);
  if (!vk_device.Create(surface)) {
    return EXIT_FAILURE;
  }

  // ========== Main loop.
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  // ========== Vulkan cleanup.
  vk_device.Destroy();
  vkDestroySurfaceKHR(vk_instance.get(), surface, nullptr);
  vk_instance.Destroy();

  // ========== glfw cleanup.
  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
