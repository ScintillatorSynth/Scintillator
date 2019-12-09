#ifndef SRC_OSCHANDLER_HPP_
#define SRC_OSCHANDLER_HPP_

#include <memory>
#include <string>

// Forward declarations from OscPack library.
class UdpListeningReceiveSocket;
class UdpTransmitSocket;

namespace scin {

class OscHandler {
public:
    OscHandler(int listenPort, const std::string& bindAddress);
    ~OscHandler();

    void run();
    void shutdown();

private:
    class OscListener;

    int m_listenPort;
    std::string m_bindAddress;

    std::unique_ptr<OscListener> m_listener;
    std::unique_ptr<UdpListeningReceiveSocket> m_listenSocket;
};

}  // namespace scin

#endif  // SRC_OSCHANDLER_HPP_

