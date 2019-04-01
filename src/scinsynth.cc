#include "vulkan/device.h"
#include "vulkan/instance.h"
#include "vulkan/pipeline.h"
#include "vulkan/scin_include_vulkan.h"
#include "vulkan/shader.h"
#include "vulkan/shader_compiler.h"
#include "vulkan/shader_source.h"
#include "vulkan/swapchain.h"
#include "vulkan/window.h"

#include <iostream>
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
            "layout(location = 0) out vec3 fragColor;\n"
            "\n"
            "vec2 positions[3] = vec2[](\n"
            "   vec2(0.0, -0.5),\n"
            "   vec2(0.5, 0.5),\n"
            "   vec2(-0.5, 0.5)\n"
            ");\n"
            "\n"
            "vec3 colors[3] = vec3[]("
            "   vec3(1.0, 0.0, 0.0),\n"
            "   vec3(0.0, 1.0, 0.0),\n"
            "   vec3(0.0, 0.0, 1.0)\n"
            ");\n"
            "\n"
            "void main() {\n"
            "   gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);\n"
            "   fragColor = colors[gl_VertexIndex];\n"
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

    scin::vk::Pipeline pipeline(device);
    if (!pipeline.Create(vertex_shader.get(), fragment_shader.get(),
            &swapchain)) {
        std::cerr << "error in pipeline creation." << std::endl;
        return EXIT_FAILURE;
    }

    if (!swapchain.CreateFramebuffers(&pipeline)) {
        std::cerr << "error creating framebuffers." << std::endl;
        return EXIT_FAILURE;
    }

    // ========== Main loop.
    window.Run();

    // ========== Vulkan cleanup.
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

