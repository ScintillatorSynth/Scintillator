#ifndef SRC_OSCHANDLER_HPP_
#define SRC_OSCHANDLER_HPP_

#include <functional>
#include <memory>
#include <string>
#include <thread>

// Forward declarations from OscPack library.
class UdpListeningReceiveSocket;
class UdpTransmitSocket;

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

class OscHandler {
public:
    OscHandler(const std::string& bindAddress, int listenPort);
    ~OscHandler();

    /*! Launches a thread to run the OscHandler main loop.
     *
     * \param logger The shared logging object.
     * \param async The async operations handler.
     * \param archetypes The archetypes dictionary.
     * \param compositor The root compositor, for issuing synchronous commands directly.
     * \param offscreen The offscreen renderer, if running in non realtime, for encode requests.
     * \param quitHandler Function to call if we receive an OSC /scin_quit command. When called, it should safely
     *        terminate the scinsynth program.
     * \return true if listening thread successfully started, false otherwise.
     */
    bool run(std::shared_ptr<Logger> logger, std::shared_ptr<Async> async, std::shared_ptr<core::Archetypes> archetypes,
             std::shared_ptr<Compositor> compositor, std::shared_ptr<vk::Offscreen> offscreen,
             std::shared_ptr<const vk::FrameTimer> frameTimer, std::function<void()> quitHandler);

    /*! Unbinds UDP socket and terminates listening thread.
     *
     * Will send a response to the client if the shutdown was ordered by an OSC /scin_quit command.
     */
    void shutdown();

private:
    class OscListener;

    std::string m_bindAddress;
    int m_listenPort;
    std::unique_ptr<OscListener> m_listener;
    std::unique_ptr<UdpListeningReceiveSocket> m_listenSocket;
    std::thread m_socketThread;
};

} // namespace scin

#endif // SRC_OSCHANDLER_HPP_
