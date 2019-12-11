#ifndef SRC_OSCHANDLER_HPP_
#define SRC_OSCHANDLER_HPP_

#include <functional>
#include <future>
#include <memory>
#include <string>

// Forward declarations from OscPack library.
class UdpListeningReceiveSocket;
class UdpTransmitSocket;

namespace scin {

class OscHandler {
public:
    OscHandler(const std::string& bindAddress, int listenPort);
    ~OscHandler();

    // Function to call if we receive an OSC /scin_quit command.
    void setQuitHandler(std::function<void()> quitHandler);
    void run();
    void shutdown();

private:
    class OscListener;

    std::string m_bindAddress;
    int m_listenPort;

    std::unique_ptr<OscListener> m_listener;
    std::unique_ptr<UdpListeningReceiveSocket> m_listenSocket;
    std::future<void> m_socketFuture;
};

}  // namespace scin

#endif  // SRC_OSCHANDLER_HPP_

