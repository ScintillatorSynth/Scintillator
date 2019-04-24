#include "LogLevels.h"
#include "OscHandler.hpp"
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
#include "Version.h"

#include "gflags/gflags.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include <memory>
#include <vector>

// Command-line options specified to gflags.
DEFINE_bool(print_version, false, "Print the Scintillator version and exit.");
DEFINE_int32(udp_port_number, -1, "A port number 0-65535.");
DEFINE_string(bind_to_address, "127.0.0.1", "Bind the UDP socket to this address.");

DEFINE_int32(log_level, 2, "Verbosity of logs, lowest value of 0 logs everything, highest value of 6 disables all "
        "logging.");

/*
DEFINE_bool(fullscreen, false, "Create a fullscreen window.");
*/
DEFINE_int32(window_width, 800, "Viewable width in pixles of window to create. Ignored if --fullscreen is supplied.");
DEFINE_int32(window_height, 600, "Viewable height in pixels of window to create. Ignored if --fullscreen is supplied.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    // Check for early exit conditions.
    if (FLAGS_print_version) {
        spdlog::info("scinsynth version {}.{}.{} from branch {} at revision {}", kScinVersionMajor, kScinVersionMinor,
            kScinVersionPatch, kScinBranch, kScinCommitHash);
        return EXIT_SUCCESS;
    }
    if (FLAGS_udp_port_number < 1024 || FLAGS_udp_port_number > 65535) {
        spdlog::error("scinsynth requires a UDP port number between 1024 and 65535. Specify with --udp_port_numnber");
        return EXIT_FAILURE;
    }

    // Set logging level, only after any critical user-triggered reporting or errors (--print_version, etc).
    scin::setGlobalLogLevel(FLAGS_log_level);

    // Start listening for incoming OSC commands on UDP.
    scin::OscHandler oscHandler(FLAGS_bind_to_address, FLAGS_udp_port_number);
    oscHandler.run();

    // ========== glfw setup.
    glfwInit();

    // ========== Vulkan setup.
    std::shared_ptr<scin::vk::Instance> instance(new scin::vk::Instance());
    if (!instance->Create()) {
        spdlog::error("unable to create Vulkan instance.");
        return EXIT_FAILURE;
    }

    scin::vk::Window window(instance);
    if (!window.Create(FLAGS_window_width, FLAGS_window_height)) {
        spdlog::error("unable to create glfw window.");
        return EXIT_FAILURE;
    }

    // Create Vulkan physical and logical device.
    std::shared_ptr<scin::vk::Device> device(new scin::vk::Device(instance));
    if (!device->Create(&window)) {
        spdlog::error("unable to create Vulkan device.");
        return EXIT_FAILURE;
    }

    // Configure swap chain based on device and surface capabilities.
    scin::vk::Swapchain swapchain(device);
    if (!swapchain.Create(&window)) {
        spdlog::error("unable to create Vulkan swapchain.");
        return EXIT_FAILURE;
    }

    scin::vk::ShaderCompiler shader_compiler;
    if (!shader_compiler.LoadCompiler()) {
        spdlog::error("unable to load shader compiler.");
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
        spdlog::error("error in fragment shader.");
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
        spdlog::error("error in pipeline creation.");
        return EXIT_FAILURE;
    }

    if (!swapchain.CreateFramebuffers(&pipeline)) {
        spdlog::error("error creating framebuffers..");
        return EXIT_FAILURE;
    }

    scin::vk::CommandPool command_pool(device);
    if (!command_pool.Create()) {
        spdlog::error("error creating command pool.");
        return EXIT_FAILURE;
    }

    const std::vector<Vertex> vertices = {
        {{ 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
        {{ 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }},
        {{ -0.5f, 0.5f }, {  0.0f, 0.0f, 1.0f }}
    };

    scin::vk::Buffer vertex_buffer(scin::vk::Buffer::kVertex, device);
    if (!vertex_buffer.Create(sizeof(Vertex) * vertices.size())) {
        spdlog::error("error creating vertex buffer.");
        return EXIT_FAILURE;
    }

    vertex_buffer.MapMemory();
    std::memcpy(vertex_buffer.mapped_address(), vertices.data(),
        sizeof(Vertex) * vertices.size());
    vertex_buffer.UnmapMemory();

    if (!command_pool.CreateCommandBuffers(&swapchain, &pipeline, &vertex_buffer)) {
        spdlog::error("error creating command buffers.");
        return EXIT_FAILURE;
    }

    if (!window.CreateSyncObjects(device.get())) {
        spdlog::error("error creating device semaphores.");
        return EXIT_FAILURE;
    }

    // ========== Main loop.
    oscHandler.setQuitHandler([&window] { window.stop(); });
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
    window.Destroy();
    instance->Destroy();

    // ========== glfw cleanup.
    glfwTerminate();

    oscHandler.shutdown();

    return EXIT_SUCCESS;
}

