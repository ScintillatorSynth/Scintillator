#include "Compositor.hpp"  // TODO: audit includes
#include "OscHandler.hpp"
#include "Version.hpp"
#include "core/FileSystem.hpp"
#include "core/LogLevels.hpp"
#include "core/Archetypes.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Instance.hpp"
#include "vulkan/Pipeline.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/ShaderCompiler.hpp"
#include "vulkan/Swapchain.hpp"
#include "vulkan/Uniform.hpp"
#include "vulkan/Vulkan.hpp"
#include "vulkan/Window.hpp"

#include "gflags/gflags.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include <future>
#include <memory>
#include <vector>

// Command-line options specified to gflags.
DEFINE_bool(print_version, false, "Print the Scintillator version and exit.");
DEFINE_int32(udp_port_number, 5511, "A port number 1024-65535.");
DEFINE_string(bind_to_address, "127.0.0.1", "Bind the UDP socket to this address.");

DEFINE_int32(log_level, 3,
             "Verbosity of logs, lowest value of 0 logs everything, highest value of 6 disables all "
             "logging.");

DEFINE_string(quark_dir, "..", "Root directory of the Scintillator Quark, for finding dependent files.");

DEFINE_int32(window_width, 800, "Viewable width in pixels of window to create. Ignored if --fullscreen is supplied.");
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
        spdlog::error("scinsynth requires a UDP port number between 1024 and 65535. Specify with --udp_port_number");
        return EXIT_FAILURE;
    }
    if (!fs::exists(FLAGS_quark_dir)) {
        spdlog::error("invalid or nonexistent path {} supplied for --quark_dir, terminating.", FLAGS_quark_dir);
        return EXIT_FAILURE;
    }

    // Set logging level, only after any critical user-triggered reporting or errors (--print_version, etc).
    scin::setGlobalLogLevel(FLAGS_log_level);

    fs::path quarkPath = fs::canonical(FLAGS_quark_dir);
    if (!fs::exists(quarkPath / "Scintillator.quark")) {
        spdlog::error("Path {} doesn't look like Scintillator Quark root directory, terminating.", quarkPath.string());
        return EXIT_FAILURE;
    }

    // Parse any built-in VGens and ScinthDefs first.
    std::shared_ptr<scin::Archetypes> archetypes(new scin::Archetypes());
    //   TODO: do serially for now
//    auto parseVGens = std::async(std::launch::async, [&archetypes, &quarkPath] {
        fs::path vgens = quarkPath / "vgens";
        spdlog::info("Parsing yaml files in {} for AbstractVGens.", vgens.string());
        for (auto entry : fs::directory_iterator(vgens)) {
            auto p = entry.path();
            if (fs::is_regular_file(p) && p.extension() == ".yaml") {
                spdlog::debug("Parsing AbstractVGen yaml file {}.", p.string());
                archetypes->loadAbstractVGensFromFile(p.string());
            }
        }
        spdlog::info("Parsed {} unique VGens.", archetypes->numberOfAbstractVGens());

        fs::path scinthDefs = quarkPath / "scinthdefs";
        spdlog::info("Parsing yaml files in {} for ScinthDefs.", scinthDefs.string());
        for (auto entry : fs::directory_iterator(scinthDefs)) {
            auto p = entry.path();
            if (fs::is_regular_file(p) && p.extension() == ".yaml") {
                spdlog::debug("Parsing ScinthDef yaml file {}.", p.string());
                archetypes->loadFromFile(p.string());
            }
        }
        spdlog::info("Parsed {} unique ScinthDefs.", archetypes->numberOfAbstractScinthDefs());
//    });

    // Start listening for incoming OSC commands on UDP.
    scin::OscHandler oscHandler(FLAGS_bind_to_address, FLAGS_udp_port_number);
    oscHandler.run();

    // ========== glfw setup.
    glfwInit();

    // ========== Vulkan setup.
    std::shared_ptr<scin::vk::Instance> instance(new scin::vk::Instance());
    if (!instance->create()) {
        spdlog::error("unable to create Vulkan instance.");
        return EXIT_FAILURE;
    }

    scin::vk::Window window(instance);
    if (!window.create(FLAGS_window_width, FLAGS_window_height)) {
        spdlog::error("unable to create glfw window.");
        return EXIT_FAILURE;
    }

    // Create Vulkan physical and logical device.
    std::shared_ptr<scin::vk::Device> device(new scin::vk::Device(instance));
    if (!device->create(&window)) {
        spdlog::error("unable to create Vulkan device.");
        return EXIT_FAILURE;
    }

    if (!window.createSwapchain(device)) {
        spdlog::error("unable to create Vulkan swapchain.");
        return EXIT_FAILURE;
    }

    std::shared_ptr<scin::Compositor> compositor(new scin::Compositor(device, window.canvas()));
    if (!compositor->create()) {
        spdlog::error("unable to create Compositor.");
        return EXIT_FAILURE;
    }

    std::shared_ptr<const scin::AbstractScinthDef> testScinthDef = archetypes->getAbstractScinthDefNamed(
            "TestScinthDef");
    compositor->buildScinthDef(testScinthDef);

    if (!window.createSyncObjects()) {
        spdlog::error("error creating device semaphores.");
        return EXIT_FAILURE;
    }

    // ========== Main loop.
    oscHandler.setQuitHandler([&window] { window.stop(); });
    window.run(compositor);

    // ========== Vulkan cleanup.
    window.destroySyncObjects();
    compositor->destroy();
    window.destroySwapchain();
    device->destroy();
    window.destroy();
    instance->destroy();

    // ========== glfw cleanup.
    glfwTerminate();

    oscHandler.shutdown();

    return EXIT_SUCCESS;
}
