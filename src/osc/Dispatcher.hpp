#ifndef SRC_OSC_DISPATCHER_HPP_
#define SRC_OSC_DISPATCHER_HPP_

#include "osc/commands/Command.hpp"

#include "lo/lo.h"

#include <array>
#include <functional>
#include <memory>
#include <string>

namespace scin {

namespace base {
class Archetypes;
}

namespace comp {
class Async;
class Compositor;
class FrameTimer;
class Offscreen;
}

namespace infra {
class CrashReporter;
class Logger;
}

namespace osc {

class Address;
class BlobMessage;

/*! The primary interface to the OSC subsystem. Manages bound ports and listening threads, and dispatches incoming OSC
 * commands for processing.
 */
class Dispatcher {
public:
    /*! Construct a Dispatcher with connections to all necessary subsystems.
     *
     * \param logger The shared logging object.
     * \param async The async operations handler.
     * \param archetypes The archetypes dictionary.
     * \param compositor The root compositor, for issuing synchronous commands directly.
     * \param offscreen The offscreen renderer, if running in non realtime, for encode requests.
     * \param quitHandler Function to call if we receive an OSC /scin_quit command. When called, it should safely
     *        terminate the scinsynth program.
     * \param crashReporter The crash reporting object, for requesting minidumps.
     */
    Dispatcher(std::shared_ptr<infra::Logger> logger, std::shared_ptr<comp::Async> async,
               std::shared_ptr<base::Archetypes> archetypes, std::shared_ptr<comp::Compositor> compositor,
               std::shared_ptr<comp::Offscreen> offscreen, std::shared_ptr<const comp::FrameTimer> frameTimer,
               std::function<void()> quitHandler, std::shared_ptr<infra::CrashReporter> crashReporter);
    ~Dispatcher();

    /*! Bind the TCP and UDP ports and set up the data structues to dispatch OSC commands for handling.
     *
     * \param bindPort Which port to open UDP and TCP listeners on.
     * \param dumpOSC If true the Dispatcher will log all incoming OSC messages.
     * \return true on success, false on failure.
     */
    bool create(const std::string& bindPort, bool dumpOSC);

    bool run();

    void stop();
    void destroy();

    // Starting call, constructs message with path.
    template <typename... Targs> void respond(lo_address address, const char* path, Targs... Fargs) {
        lo_message message = lo_message_new();
        respond(address, path, message, Fargs...);
    }
    // Recursive call specializations, adds next argument to message.
    template <typename... Targs>
    void respond(lo_address address, const char* path, lo_message message, int32_t value, Targs... Fargs) {
        lo_message_add_int32(message, value);
        respond(address, path, message, Fargs...);
    }
    template <typename... Targs>
    void respond(lo_address address, const char* path, lo_message message, const char* value, Targs... Fargs) {
        lo_message_add_string(message, value);
        respond(address, path, message, Fargs...);
    }
    template <typename... Targs>
    void respond(lo_address address, const char* path, lo_message message, double value, Targs... Fargs) {
        lo_message_add_double(message, value);
        respond(address, path, message, Fargs...);
    }
    template <typename... Targs>
    void respond(lo_address address, const char* path, lo_message message, bool value, Targs... Fargs) {
        if (value) {
            lo_message_add_true(message);
        } else {
            lo_message_add_false(message);
        }
        respond(address, path, message, Fargs...);
    }
    // Base call, finishes the message and sends it on appropriate server thread.
    void respond(lo_address address, const char* path, lo_message message) {
        if (lo_address_get_protocol(address) == LO_TCP) {
            lo_send_message_from(address, m_tcpServer, path, message);
        } else {
            lo_send_message_from(address, m_udpServer, path, message);
        }
        lo_message_free(message);
    }

    // Accessor methods primarily used by Command subclasses to talk to rest of scintillator subsystems.
    std::shared_ptr<infra::Logger> logger() { return m_logger; }
    std::shared_ptr<comp::Async> async() { return m_async; }
    std::shared_ptr<base::Archetypes> archetypes() { return m_archetypes; }
    std::shared_ptr<comp::Compositor> compositor() { return m_compositor; }
    std::shared_ptr<comp::Offscreen> offscreen() { return m_offscreen; }
    std::shared_ptr<const comp::FrameTimer> frameTimer() { return m_frameTimer; }
    std::shared_ptr<infra::CrashReporter> crashReporter() { return m_crashReporter; }
    void callQuitHandler(std::shared_ptr<Address> quitOrigin);
    void setDumpOSC(bool enable) { m_dumpOSC = enable; }
    void processMessageFrom(lo_address address, std::shared_ptr<BlobMessage> onCompletion);

private:
    static void loError(int number, const char* message, const char* path);
    static int loHandle(const char* path, const char* types, lo_arg** argv, int argc, lo_message message,
                        void* userData);
    void dispatch(const char* path, int argc, lo_arg** argv, const char* types, lo_address address);

    std::shared_ptr<infra::Logger> m_logger;
    std::shared_ptr<comp::Async> m_async;
    std::shared_ptr<base::Archetypes> m_archetypes;
    std::shared_ptr<comp::Compositor> m_compositor;
    std::shared_ptr<comp::Offscreen> m_offscreen;
    std::shared_ptr<const comp::FrameTimer> m_frameTimer;
    std::function<void()> m_quitHandler;
    std::shared_ptr<infra::CrashReporter> m_crashReporter;
    std::array<std::unique_ptr<commands::Command>, commands::Command::Number::kCommandCount> m_commands;

    lo_server_thread m_tcpThread;
    lo_server m_tcpServer;
    lo_server_thread m_udpThread;
    lo_server m_udpServer;
    std::shared_ptr<Address> m_quitOrigin;
    bool m_dumpOSC;
};

} // namespace osc

} // namespace scin

#endif // SRC_OSC_DISPATCHER_HPP_
