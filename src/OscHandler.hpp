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

class Archetypes;
class Async;
class Compositor;

class OscHandler {
public:
    OscHandler(const std::string& bindAddress, int listenPort);
    ~OscHandler();

    /*! Launches a thread to run the OscHandler main loop.
     *
     * \param async The async operations handler.
     * \param archetypes The archetypes dictionary.
     * \param compositor The root compositor, for issuing synchronous commands directly.
     * \param quitHandler Function to call if we receive an OSC /scin_quit command. When called, it should safely
     *        terminate the scinsynth program.
     */
    void run(std::shared_ptr<Async> async, std::shared_ptr<Archetypes> archetypes,
             std::shared_ptr<Compositor> compositor, std::function<void()> quitHandler);

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
