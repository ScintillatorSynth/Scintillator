#include "osc/Dispatcher.hpp"

#include "Async.hpp"
#include "Compositor.hpp"
#include "Logger.hpp"
#include "Version.hpp"
#include "av/ImageEncoder.hpp"
#include "core/Archetypes.hpp"
#include "osc/commands/AdvanceFrame.hpp"
#include "osc/commands/DefFree.hpp"
#include "osc/commands/DefLoad.hpp"
#include "osc/commands/DefLoadDir.hpp"
#include "osc/commands/DefReceive.hpp"
#include "osc/commands/DumpOSC.hpp"
#include "osc/commands/LogLevel.hpp"
#include "osc/commands/NodeFree.hpp"
#include "osc/commands/NodeRun.hpp"
#include "osc/commands/Notify.hpp"
#include "osc/commands/Quit.hpp"
#include "osc/commands/ScinthNew.hpp"
#include "osc/commands/ScreenShot.hpp"
#include "osc/commands/Status.hpp"
#include "osc/commands/Sync.hpp"
#include "osc/commands/Version.hpp"
#include "vulkan/FrameTimer.hpp"
#include "vulkan/Offscreen.hpp"

#include "spdlog/spdlog.h"

#include <cstring>

namespace scin { namespace osc {

Dispatcher::Dispatcher(std::shared_ptr<Logger> logger, std::shared_ptr<Async> async,
                       std::shared_ptr<core::Archetypes> archetypes, std::shared_ptr<Compositor> compositor,
                       std::shared_ptr<vk::Offscreen> offscreen, std::shared_ptr<const vk::FrameTimer> frameTimer,
                       std::function<void()> quitHandler):
    m_logger(logger),
    m_async(async),
    m_archetypes(archetypes),
    m_compositor(compositor),
    m_offscreen(offscreen),
    m_frameTimer(frameTimer),
    m_quitHandler(quitHandler),
    m_tcpThread(nullptr),
    m_tcpServer(nullptr),
    m_udpThread(nullptr),
    m_udpServer(nullptr) {}

Dispatcher::~Dispatcher() { destroy(); }

bool Dispatcher::create(const std::string& bindPort) {
    m_tcpThread = lo_server_thread_new_with_proto(bindPort.data(), LO_TCP, loError);
    if (!m_tcpThread) {
        spdlog::error("Unable to create OSC listener on TCP port {}", bindPort);
        return false;
    }

    m_udpThread = lo_server_thread_new_with_proto(bindPort.data(), LO_UDP, loError);
    if (!m_udpThread) {
        spdlog::error("Unable to create OSC listener on UDP port {}", bindPort);
        return false;
    }

    // We use the same handler and our own pattern matching to dispatch all commands so we only add the one handler
    // to lo and set it to match all incoming patterns.
    lo_server_thread_add_method(m_tcpThread, nullptr, nullptr, loHandle, this);
    lo_server_thread_add_method(m_udpThread, nullptr, nullptr, loHandle, this);

    // Populate command array.
    m_commands[commands::Command::kNotify].reset(new commands::Notify(this));
    m_commands[commands::Command::kStatus].reset(new commands::Status(this));
    m_commands[commands::Command::kQuit].reset(new commands::Quit(this));
    m_commands[commands::Command::kDumpOSC].reset(new commands::DumpOSC(this));
    m_commands[commands::Command::kSync].reset(new commands::Sync(this));
    m_commands[commands::Command::kLogLevel].reset(new commands::LogLevel(this));
    m_commands[commands::Command::kVersion].reset(new commands::Version(this));
    m_commands[commands::Command::kDRecv].reset(new commands::DefReceive(this));
    m_commands[commands::Command::kDLoad].reset(new commands::DefLoad(this));
    m_commands[commands::Command::kDLoadDir].reset(new commands::DefLoadDir(this));
    m_commands[commands::Command::kDFree].reset(new commands::DefFree(this));
    m_commands[commands::Command::kNFree].reset(new commands::NodeFree(this));
    m_commands[commands::Command::kNRun].reset(new commands::NodeRun(this));
    m_commands[commands::Command::kSNew].reset(new commands::ScinthNew(this));
    m_commands[commands::Command::kNRTScreenShot].reset(new commands::ScreenShot(this));
    m_commands[commands::Command::kNRTAdvanceFrame].reset(new commands::AdvanceFrame(this));

    return true;
}

bool Dispatcher::run() {
    if (lo_server_thread_start(m_tcpThread) < 0) {
        spdlog::error("Failed to start OSC dispatcher TCP thread.");
        return false;
    }

    if (lo_server_thread_start(m_udpThread) < 0) {
        spdlog::error("Failed to start OSC dispatcher UDP thread.");
        return false;
    }

    return true;
}

void Dispatcher::stop() {
    if (lo_server_thread_stop(m_tcpThread) < 0) {
        spdlog::error("Failed to stop OSC dispatcher TCP thread.");
    }
    if (lo_server_thread_stop(m_udpThread) < 0) {
        spdlog::error("Failed to stop OSC dispatcher UDP thread.");
    }
}

void Dispatcher::destroy() {
    if (m_tcpThread) {
        lo_server_thread_free(m_tcpThread);
        m_tcpThread = nullptr;
    }
    if (m_udpThread) {
        lo_server_thread_free(m_udpThread);
        m_udpThread = nullptr;
    }
}

// static
void Dispatcher::loError(int number, const char* message, const char* path) {
    spdlog::error("lo error number: {}, message: {}, path: {}", number, message, path);
}

// static
int Dispatcher::loHandle(const char* path, const char* types, lo_arg** argv, int argc, lo_message message,
                         void* userData) {
    // All scinsynth messages start with a scin_ prefix, to avoid confusion with similar messages and responses
    // that are sent to scsynth. So we check or this prefix first, and ignore any messages that lack it.
    const char* kPrefix = "/scin_";
    const size_t kPrefixLength = 6;
    size_t length = std::strlen(path);
    if (length < kPrefixLength || std::strncmp(path, kPrefix, kPrefixLength) != 0) {
        spdlog::error("OSC command {} does not have {} prefix, is not a scinsynth command.", path, kPrefix);
        return 0;
    }

    commands::Command::Number number = commands::Command::getNumberNamed(path, length);
    Dispatcher* dispatcher = static_cast<Dispatcher*>(userData);

    if (!dispatcher->m_commands[number]) {
        spdlog::error("Received unsupported OSC command {}", path);
        return 0;
    }

    dispatcher->m_commands[number]->processMessage(argc, argv, types, message);
    return 0;
}

} // namespace osc
} // namespace scin
