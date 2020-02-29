#include "av/AVIncludes.hpp"
#include "base/Archetypes.hpp"
#include "base/FileSystem.hpp"
#include "comp/Async.hpp" // TODO: audit includes
#include "comp/Compositor.hpp"
#include "comp/FrameTimer.hpp"
#include "comp/Offscreen.hpp"
#include "comp/Pipeline.hpp"
#include "comp/Window.hpp"
#include "infra/Logger.hpp"
#include "infra/Version.hpp"
#include "osc/Dispatcher.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/DeviceChooser.hpp"
#include "vulkan/Instance.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/Uniform.hpp"
#include "vulkan/Vulkan.hpp"

#include "fmt/core.h"
#include "gflags/gflags.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include <chrono>
#include <memory>
#include <vector>

// Command-line options specified to gflags.
DEFINE_bool(print_version, false, "Print the Scintillator version and exit.");
DEFINE_bool(print_devices, false, "Print the detected devices and exit.");
DEFINE_string(port_number, "5511", "A port number 1024-65535.");
DEFINE_bool(dump_osc, false, "Start dumping OSC messages immediately on bootup.");
// TODO: find a way to get liblo to bind less widely.
// DEFINE_string(bind_to_address, "127.0.0.1", "Bind the UDP socket to this address.");

DEFINE_int32(log_level, 3,
             "Verbosity of logs, lowest value of 0 logs everything, highest value of 6 disables all "
             "logging.");

DEFINE_string(quark_dir, "..", "Root directory of the Scintillator Quark, for finding dependent files.");

DEFINE_int32(width, 800, "Viewable width in pixels of window to create. Ignored if --fullscreen is supplied.");
DEFINE_int32(height, 600, "Viewable height in pixels of window to create. Ignored if --fullscreen is supplied.");
DEFINE_bool(keep_on_top, true, "If true will keep the window on top of other windows with focus.");

DEFINE_int32(async_worker_threads, 2,
             "Number of threads to reserve for asynchronous operations (like loading ScinthDefs).");

DEFINE_bool(vulkan_validation, true, "Enable Vulkan validation layers.");

DEFINE_int32(frame_rate, -1,
             "Target framerate in frames per second. Negative number means track windowing system "
             "framerate. Zero means non-interactive. Positive means to render at that frame per second rate.");
DEFINE_bool(create_window, true, "If false, Scintillator will not create a window.");

DEFINE_string(device_name, "",
              "If empty, will pick the highest performance device available. Otherwise, should be as "
              "many characters of the name as needed to uniquely identify the device.");
DEFINE_bool(swiftshader, false,
            "If true, ignores --device_name and will always match the swiftshader device, if present.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    std::shared_ptr<scin::infra::Logger> logger(new scin::infra::Logger());
    logger->initLogging(FLAGS_log_level);

    // Check for early exit conditions.
    std::string version = fmt::format("scinsynth version {}.{}.{} from branch {} at revision {}", kScinVersionMajor,
                                      kScinVersionMinor, kScinVersionPatch, kScinBranch, kScinCommitHash);
    if (FLAGS_print_version) {
        fmt::print(version);
        return EXIT_SUCCESS;
    }
    if (!fs::exists(FLAGS_quark_dir)) {
        fmt::print("invalid or nonexistent path {} supplied for --quark_dir, terminating.", FLAGS_quark_dir);
        return EXIT_FAILURE;
    }

    fs::path quarkPath = fs::canonical(FLAGS_quark_dir);
    if (!fs::exists(quarkPath / "Scintillator.quark")) {
        spdlog::error("Path {} doesn't look like Scintillator Quark root directory, terminating.", quarkPath.string());
        return EXIT_FAILURE;
    }

    spdlog::info(version);

    // ========== Ask libavcodec to register encoders and decoders, required for older libavcodecs.
#if LIBAVCODEC_VERSION_MAJOR < 58
    av_register_all();
#endif

    // ========== glfw setup, this also loads Vulkan for us via the Vulkan-Loader.
    glfwInit();
    if (FLAGS_create_window && (glfwVulkanSupported() != GLFW_TRUE)) {
        spdlog::error("This platform does not support Vulkan!");
        return EXIT_FAILURE;
    }

    // ========== Vulkan setup.
    std::shared_ptr<scin::vk::Instance> instance(new scin::vk::Instance(FLAGS_vulkan_validation));
    if (!instance->create()) {
        spdlog::error("scinsynth unable to create Vulkan instance.");
        return EXIT_FAILURE;
    }

    scin::vk::DeviceChooser chooser(instance);
    chooser.enumerateAllDevices();
    std::string deviceReport = fmt::format("found {} Vulkan devices:\n", chooser.devices().size());
    for (auto info : chooser.devices()) {
        deviceReport += fmt::format("  name: {}, type: {}, vendorID: {:x}, deviceID: {:x}\n", info.name(),
                                    info.typeName(), info.vendorID(), info.deviceID());
    }
    if (FLAGS_print_devices) {
        fmt::print(deviceReport);
        return EXIT_SUCCESS;
    } else {
        spdlog::info(deviceReport);
    }

    // Create Vulkan physical and logical device.
    std::shared_ptr<scin::vk::Device> device;
    // If swiftshader was selected prefer it over all other device selection arguments.
    if (FLAGS_swiftshader) {
        for (auto info : chooser.devices()) {
            if (info.isSwiftShader()) {
                spdlog::info("Selecting SwiftShader device.");
                device.reset(new scin::vk::Device(instance, info));
                break;
            }
        }
        if (!device) {
            spdlog::error("SwiftShader requested but device not found.");
            return EXIT_FAILURE;
        }
    } else {
        if (FLAGS_device_name.size()) {
            for (auto info : chooser.devices()) {
                if (std::strncmp(FLAGS_device_name.data(), info.name(), FLAGS_device_name.size()) == 0) {
                    if (FLAGS_create_window && !info.supportsWindow()) {
                        spdlog::error("Device name {}, does not support rendering to a window.", info.name());
                        return EXIT_FAILURE;
                    }
                    spdlog::info("Device name {} match, selecting {}", FLAGS_device_name, info.name());
                    device.reset(new scin::vk::Device(instance, info));
                    // break;
                }
            }
        }
    }

    if (!device && chooser.bestDeviceIndex() >= 0) {
        auto info = chooser.devices().at(chooser.bestDeviceIndex());
        if (FLAGS_create_window && !info.supportsWindow()) {
            spdlog::error("Automatically chosen device {} doesn't support window rendering.", info.name());
            return EXIT_FAILURE;
        }
        spdlog::info("Choosing fastest device class {}, device {}", info.typeName(), info.name());
        device.reset(new scin::vk::Device(instance, info));
    }

    if (!device) {
        spdlog::error("failed to find a suitable Vulkan device.");
        return EXIT_FAILURE;
    }
    if (!device->create(FLAGS_create_window)) {
        spdlog::error("unable to create Vulkan device.");
        return EXIT_FAILURE;
    }

    std::shared_ptr<scin::comp::Window> window;
    std::shared_ptr<scin::comp::Canvas> canvas;
    std::shared_ptr<scin::comp::Offscreen> offscreen;
    std::shared_ptr<const scin::comp::FrameTimer> frameTimer;
    if (FLAGS_create_window) {
        window.reset(
            new scin::comp::Window(instance, device, FLAGS_width, FLAGS_height, FLAGS_keep_on_top, FLAGS_frame_rate));
        if (!window->create()) {
            spdlog::error("Failed to create window.");
            return EXIT_FAILURE;
        }
        canvas = window->canvas();
        offscreen = window->offscreen();
        frameTimer = window->frameTimer();
    } else {
        offscreen.reset(new scin::comp::Offscreen(device, FLAGS_width, FLAGS_height, FLAGS_frame_rate));
        if (!offscreen->create(3)) {
            spdlog::error("Failed to create offscreen renderer.");
            return EXIT_FAILURE;
        }
        canvas = offscreen->canvas();
        frameTimer = offscreen->frameTimer();
    }

    std::shared_ptr<scin::comp::Compositor> compositor(new scin::comp::Compositor(device, canvas));
    if (!compositor->create()) {
        spdlog::error("unable to create Compositor.");
        return EXIT_FAILURE;
    }

    std::shared_ptr<scin::base::Archetypes> archetypes(new scin::base::Archetypes());
    std::shared_ptr<scin::comp::Async> async(new scin::comp::Async(archetypes, compositor, device));
    async->run(FLAGS_async_worker_threads);

    // Chain async calls to load VGens, then ScinthDefs.
    async->vgenLoadDirectory(quarkPath / "vgens", [async, &quarkPath, compositor](int) {
        async->scinthDefLoadDirectory(quarkPath / "scinthdefs",
                                      [](int) { spdlog::info("finished loading predefined VGens and ScinthDefs."); });
    });

    std::function<void()> quitHandler;
    if (FLAGS_create_window) {
        quitHandler = [window] { window->stop(); };
    } else {
        quitHandler = [offscreen] { offscreen->stop(); };
    }
    scin::osc::Dispatcher dispatcher(logger, async, archetypes, compositor, offscreen, frameTimer, quitHandler);
    if (!dispatcher.create(FLAGS_port_number, FLAGS_dump_osc)) {
        spdlog::error("Failed creating OSC command dispatcher.");
        return EXIT_FAILURE;
    }
    if (!dispatcher.run()) {
        spdlog::error("Failed starting OSC communications threads.");
        return EXIT_FAILURE;
    }

    // ========== Main loop.
    if (FLAGS_create_window) {
        window->run(compositor);
    } else {
        offscreen->run(compositor);
    }

    dispatcher.stop();
    dispatcher.destroy();

    // ========== Vulkan cleanup.
    async->stop();
    compositor->destroy();
    if (FLAGS_create_window) {
        window->destroy();
    } else {
        offscreen->destroy();
    }
    device->destroy();
    instance->destroy();

    // ========== glfw cleanup.
    glfwTerminate();

    spdlog::info("scinsynth exited normally.");
    return EXIT_SUCCESS;
}
