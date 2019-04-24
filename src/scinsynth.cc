#include "vulkan/buffer.h"
#include "vulkan/command_pool.h"
#include "vulkan/device.h"
#include "vulkan/instance.h"
#include "vulkan/pipeline.h"
#include "vulkan/scin_include_vulkan.h"
#include "vulkan/shader.h"
#include "vulkan/shader_compiler.h"
#include "vulkan/shader_source.h"
#include "vulkan/swapchain.h"
#include "vulkan/window.h"

#include <glm/glm.hpp>

#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    // ========== glfw setup.
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
    scin::vk::Swapchain swapchain(device);
    if (!swapchain.Create(&window)) {
        return EXIT_FAILURE;
    }

    scin::vk::ShaderCompiler shader_compiler;
    if (!shader_compiler.LoadCompiler()) {
        return EXIT_FAILURE;
    }

    scin::vk::ShaderSource vertex_source("vertex shader",
            "#version 450\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "\n"
            "layout(location = 0) in vec2 inPosition;\n"
            "layout(location = 1) in vec3 inColor;\n"
            "\n"
            "layout(location = 0) out vec3 fragColor;\n"
            "\n"
            "void main() {\n"
            "   gl_Position = vec4(inPosition, 0.0, 1.0);\n"
            "   fragColor = inColor;\n"
            "}\n"
    );
    std::unique_ptr<scin::vk::Shader> vertex_shader = shader_compiler.Compile(
            device, &vertex_source, scin::vk::Shader::kVertex);
    if (!vertex_shader) {
        return EXIT_FAILURE;
    }

    scin::vk::ShaderSource fragment_source("fragment shader",
            "#version 450\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "\n"
            "layout(location = 0) in vec3 fragColor;\n"
            "\n"
            "layout(location = 0) out vec4 outColor;\n"
            "\n"
            "void main() {\n"
            "   outColor = vec4(fragColor, 1.0);\n"
            "}\n"
    );
    std::unique_ptr<scin::vk::Shader> fragment_shader = shader_compiler.Compile(
            device, &fragment_source, scin::vk::Shader::kFragment);
    if (!fragment_shader) {
        std::cerr << "error in fragment shader." << std::endl;
        return EXIT_FAILURE;
    }

    shader_compiler.ReleaseCompiler();

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;
    };

    scin::vk::Pipeline pipeline(device);
    pipeline.SetVertexStride(sizeof(Vertex));
    pipeline.AddVertexAttribute(scin::vk::Pipeline::kVec2, offsetof(Vertex, pos));
    pipeline.AddVertexAttribute(scin::vk::Pipeline::kVec3, offsetof(Vertex, color));

    if (!pipeline.Create(vertex_shader.get(), fragment_shader.get(),
            &swapchain)) {
        std::cerr << "error in pipeline creation." << std::endl;
        return EXIT_FAILURE;
    }

    if (!swapchain.CreateFramebuffers(&pipeline)) {
        std::cerr << "error creating framebuffers." << std::endl;
        return EXIT_FAILURE;
    }

    scin::vk::CommandPool command_pool(device);
    if (!command_pool.Create()) {
        std::cerr << "error creating command pool." << std::endl;
        return EXIT_FAILURE;
    }

    const std::vector<Vertex> vertices = {
        {{ 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
        {{ 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }},
        {{ -0.5f, 0.5f }, {  0.0f, 0.0f, 1.0f }}
    };

    scin::vk::Buffer vertex_buffer(scin::vk::Buffer::kVertex, device);
    if (!vertex_buffer.Create(sizeof(Vertex) * vertices.size())) {
        std::cerr << "error creating vertex buffer." << std::endl;
        return EXIT_FAILURE;
    }

    vertex_buffer.MapMemory();
    std::memcpy(vertex_buffer.mapped_address(), vertices.data(),
        sizeof(Vertex) * vertices.size());
    vertex_buffer.UnmapMemory();

    if (!command_pool.CreateCommandBuffers(&swapchain, &pipeline, &vertex_buffer)) {
        std::cerr << "error creating command buffers." << std::endl;
        return EXIT_FAILURE;
    }

    if (!window.CreateSyncObjects(device.get())) {
        std::cerr << "error creating semaphores." << std::endl;
        return EXIT_FAILURE;
    }

    // ========== Main loop.
    window.Run(device.get(), &swapchain, &command_pool);

    // ========== Vulkan cleanup.
    window.DestroySyncObjects(device.get());
    command_pool.Destroy();
    vertex_buffer.Destroy();
    swapchain.DestroyFramebuffers();
    pipeline.Destroy();
    vertex_shader->Destroy();
    fragment_shader->Destroy();
    swapchain.Destroy();
    device->Destroy();
    instance->Destroy();

    // ========== glfw cleanup.
    glfwTerminate();

    return EXIT_SUCCESS;
}

