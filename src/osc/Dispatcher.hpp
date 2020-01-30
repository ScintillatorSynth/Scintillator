#ifndef SRC_OSC_DISPATCHER_HPP_
#define SRC_OSC_DISPATCHER_HPP_

#include "osc/commands/Command.hpp"

#include "lo/lo.h"

#include <array>
#include <functional>
#include <memory>
#include <string>

namespace scin {

class Async;
class Compositor;
class Logger;

namespace core {
class Archetypes;
}

namespace vk {
class FrameTimer;
class Offscreen;
}

namespace osc {

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
     */
    Dispatcher(std::shared_ptr<Logger> logger, std::shared_ptr<Async> async,
               std::shared_ptr<core::Archetypes> archetypes, std::shared_ptr<Compositor> compositor,
               std::shared_ptr<vk::Offscreen> offscreen, std::shared_ptr<const vk::FrameTimer> frameTimer,
               std::function<void()> quitHandler);
    ~Dispatcher();

    /*! Bind the TCP and UDP ports and set up the data structues to dispatch OSC commands for handling.
     *
     * \param bindPort Which port to open UDP and TCP listeners on.
     * \return true on success, false on failure.
     */
    bool create(const std::string& bindPort);

    bool run();

    void stop();
    void destroy();

    std::shared_ptr<Logger> logger() { return m_logger; }
    std::shared_ptr<Async> async() { return m_async; }
    std::shared_ptr<core::Archetypes> archetypes() { return m_archetypes; }
    std::shared_ptr<Compositor> compositor() { return m_compositor; }
    std::shared_ptr<vk::Offscreen> offscreen() { return m_offscreen; }
    std::shared_ptr<const vk::FrameTimer> frameTimer() { return m_frameTimer; }
    std::function<void()> quitHandler() { return m_quitHandler; }

private:
    static void loError(int number, const char* message, const char* path);
    static int loHandle(const char* path, const char* types, lo_arg** argv, int argc, lo_message message,
                        void* userData);

    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<Async> m_async;
    std::shared_ptr<core::Archetypes> m_archetypes;
    std::shared_ptr<Compositor> m_compositor;
    std::shared_ptr<vk::Offscreen> m_offscreen;
    std::shared_ptr<const vk::FrameTimer> m_frameTimer;
    std::function<void()> m_quitHandler;
    std::array<std::unique_ptr<commands::Command>, commands::Command::Number::kCommandCount> m_commands;

    lo_server_thread m_tcpThread;
    lo_server m_tcpServer;
    lo_server_thread m_udpThread;
    lo_server m_udpServer;
};

} // namespace osc

} // namespace scin

#endif // SRC_OSC_DISPATCHER_HPP_
