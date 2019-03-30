#include "vulkan/window.h"

#include "vulkan/instance.h"

#include <iostream>

namespace scin {

namespace vk {

Window::Window(std::shared_ptr<Instance> instance) :
    instance_(instance),
    width_(-1),
    height_(-1),
    window_(nullptr),
    surface_(VK_NULL_HANDLE) {
}

Window::~Window() {
}

bool Window::Create(int width, int height) {
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


void Window::Run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
    }
}

void Window::Destroy() {
    vkDestroySurfaceKHR(instance_->get(), surface_, nullptr);
    glfwDestroyWindow(window_);
}

}    // namespace vk

}    // namespace scin

