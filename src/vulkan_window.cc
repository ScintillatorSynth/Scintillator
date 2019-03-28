#include "vulkan_window.h"

#include "vulkan_instance.h"

#include <iostream>

namespace scin {

VulkanWindow::VulkanWindow(std::shared_ptr<VulkanInstance> instance) :
    instance_(instance),
    width_(-1),
    height_(-1),
    window_(nullptr),
    surface_(VK_NULL_HANDLE) {
}

VulkanWindow::~VulkanWindow() {
}

bool VulkanWindow::Create(int width, int height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(width, height,
            "ScintillatorSynth", nullptr, nullptr);
    if (glfwCreateWindowSurface(instance_->get(), window_, nullptr, &surface_)
            != VK_SUCCESS) {
        std::cerr << "failed to create surface" << std::endl;
        return false;
    }
    width_ = width;
    height_ = height;

    return true;
}


void VulkanWindow::Run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
    }
}

void VulkanWindow::Destroy() {
    vkDestroySurfaceKHR(instance_->get(), surface_, nullptr);
    glfwDestroyWindow(window_);
}

}  // namespace scin

