#include "osc/Dispatcher.hpp"

#include "Async.hpp"
#include "Compositor.hpp"
#include "Logger.hpp"
#include "Version.hpp"
#include "av/ImageEncoder.hpp"
#include "core/Archetypes.hpp"
#include "osc/Address.hpp"
#include "osc/BlobMessage.hpp"
#include "osc/commands/AdvanceFrame.hpp"
#include "osc/commands/DefFree.hpp"
#include "osc/commands/DefLoad.hpp"
#include "osc/commands/DefLoadDir.hpp"
#include "osc/commands/DefReceive.hpp"
#include "osc/commands/DumpOSC.hpp"
#include "osc/commands/Echo.hpp"
#include "osc/commands/LogAppend.hpp"
#include "osc/commands/LogLevel.hpp"
#include "osc/commands/NodeFree.hpp"
#include "osc/commands/NodeRun.hpp"
#include "osc/commands/Notify.hpp"
#include "osc/commands/Quit.hpp"
#include "osc/commands/ScinVersion.hpp"
#include "osc/commands/ScinthNew.hpp"
#include "osc/commands/ScreenShot.hpp"
#include "osc/commands/SleepFor.hpp"
#include "osc/commands/Status.hpp"
#include "osc/commands/Sync.hpp"
#include "vulkan/FrameTimer.hpp"
#include "vulkan/Offscreen.hpp"

#include "fmt/core.h"
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
    m_udpServer(nullptr),
    m_quitOrigin(nullptr),
    m_dumpOSC(false) {}

Dispatcher::~Dispatcher() { destroy(); }

bool Dispatcher::create(const std::string& bindPort, bool dumpOSC) {
    m_dumpOSC = dumpOSC;
    m_tcpThread = lo_server_thread_new_with_proto(bindPort.data(), LO_TCP, loError);
    if (!m_tcpThread) {
        spdlog::error("Unable to create OSC listener on TCP port {}", bindPort);
        return false;
    }
    m_tcpServer = lo_server_thread_get_server(m_tcpThread);

    m_udpThread = lo_server_thread_new_with_proto(bindPort.data(), LO_UDP, loError);
    if (!m_udpThread) {
        spdlog::error("Unable to create OSC listener on UDP port {}", bindPort);
        return false;
    }
    m_udpServer = lo_server_thread_get_server(m_udpThread);

    // We use the same handler and our own pattern matching to dispatch all commands so we only add the one handler
    // per server and set it to match all incoming patterns.
    lo_server_thread_add_method(m_tcpThread, nullptr, nullptr, loHandle, this);
    lo_server_thread_add_method(m_udpThread, nullptr, nullptr, loHandle, this);

    // Populate command array.
    m_commands[commands::Command::kNotify].reset(new commands::Notify(this));
    m_commands[commands::Command::kStatus].reset(new commands::Status(this));
    m_commands[commands::Command::kQuit].reset(new commands::Quit(this));
    m_commands[commands::Command::kDumpOSC].reset(new commands::DumpOSC(this));
    m_commands[commands::Command::kSync].reset(new commands::Sync(this));
    m_commands[commands::Command::kLogLevel].reset(new commands::LogLevel(this));
    m_commands[commands::Command::kVersion].reset(new commands::ScinVersion(this));
    m_commands[commands::Command::kDRecv].reset(new commands::DefReceive(this));
    m_commands[commands::Command::kDLoad].reset(new commands::DefLoad(this));
    m_commands[commands::Command::kDLoadDir].reset(new commands::DefLoadDir(this));
    m_commands[commands::Command::kDFree].reset(new commands::DefFree(this));
    m_commands[commands::Command::kNFree].reset(new commands::NodeFree(this));
    m_commands[commands::Command::kNRun].reset(new commands::NodeRun(this));
    m_commands[commands::Command::kSNew].reset(new commands::ScinthNew(this));
    m_commands[commands::Command::kNRTScreenShot].reset(new commands::ScreenShot(this));
    m_commands[commands::Command::kNRTAdvanceFrame].reset(new commands::AdvanceFrame(this));
    m_commands[commands::Command::kEcho].reset(new commands::Echo(this));
    m_commands[commands::Command::kLogAppend].reset(new commands::LogAppend(this));
    m_commands[commands::Command::kSleepFor].reset(new commands::SleepFor(this));

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
    if (m_quitOrigin) {
        respond(m_quitOrigin->get(), "/scin_done");
    }
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

void Dispatcher::callQuitHandler(std::shared_ptr<Address> quitOrigin) {
    m_quitOrigin = quitOrigin;
    m_quitHandler();
}

void Dispatcher::processMessageFrom(lo_address address, std::shared_ptr<BlobMessage> blobMessage) {
    lo_message message = blobMessage->message();
    dispatch(blobMessage->path(), lo_message_get_argc(message), lo_message_get_argv(message),
            lo_message_get_types(message), address);
}

// static
void Dispatcher::loError(int number, const char* message, const char* path) {
    spdlog::error("lo error number: {}, message: {}, path: {}", number, message, path);
}

// static
int Dispatcher::loHandle(const char* path, const char* types, lo_arg** argv, int argc, lo_message message,
                         void* userData) {
    Dispatcher* dispatcher = static_cast<Dispatcher*>(userData);
    lo_address address = lo_message_get_source(message);
    dispatcher->dispatch(path, argc, argv, types, address);
    return 0;
}

void Dispatcher::dispatch(const char* path, int argc, lo_arg** argv, const char* types, lo_address address) {
    if (m_dumpOSC) {
        std::string osc = fmt::format("OSC: [ {}", path);
        for (int i = 0; i < argc; ++i) {
            switch (types[i]) {
            case LO_INT32:
                osc += fmt::format(", {}", *reinterpret_cast<int32_t*>(argv[i]));
                break;

            case LO_FLOAT:
                osc += fmt::format(", {}", *reinterpret_cast<float*>(argv[i]));
                break;

            case LO_STRING:
                osc += fmt::format(", {}", reinterpret_cast<const char*>(argv[i]));
                break;

            case LO_BLOB:
                osc += std::string(", <binary blob>");
                break;

            default:
                osc += fmt::format(", <unrecognized type {}>", types[i]);
                break;
            }
        }
        osc += " ]";
        spdlog::info(osc);
    }

    // All scinsynth messages start with a scin_ prefix, to avoid confusion with similar messages and responses
    // that are sent to scsynth. So we check or this prefix first, and ignore any messages that lack it.
    const char* kPrefix = "/scin_";
    const size_t kPrefixLength = 6;
    size_t length = std::strlen(path);
    if (length < kPrefixLength || std::strncmp(path, kPrefix, kPrefixLength) != 0) {
        spdlog::error("OSC command {} does not have {} prefix, is not a scinsynth command.", path, kPrefix);
    }

    commands::Command::Number number = commands::Command::getNumberNamed(path + kPrefixLength, length - kPrefixLength);

    if (m_commands[number]) {
        m_commands[number]->processMessage(argc, argv, types, address);
    } else {
        spdlog::error("Received unsupported OSC command {}, number: {}", path, number);
    }
}

} // namespace osc
} // namespace scin
