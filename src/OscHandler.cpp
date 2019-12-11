#include "OscHandler.hpp"

#include "OscCommand.h"

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "osc/OscReceivedElements.h"
#include "spdlog/spdlog.h"

#include <cstring>

namespace scin {

class OscHandler::OscListener : public osc::OscPacketListener {
public:
    OscListener(OscHandler* handler) :
        osc::OscPacketListener(),
        m_handler(handler),
        m_quitHandler([]{}) {
    }

    void ProcessMessage(const osc::ReceivedMessage& message, const IpEndpointName& endpoint) override {
        try {
            const char* kPrefix = "/scin_";
            const size_t kPrefixLength = 6;
            // All scinsynth messages start with a scin_ prefix, to avoid confusion with similar messages and responses
            // that are sent to scsynth. So check for this prefix first, and ignore any messages that lack it.
            const char* command = message.AddressPattern();
            size_t commandLength = std::strlen(command);
            if (commandLength < kPrefixLength || std::strncmp(command, kPrefix, kPrefixLength) != 0) {
                spdlog::error("OSC command {} does not have {} prefix, is not a scinsynth command.", command, kPrefix);
                return;
            }

            char buf[32];
            endpoint.AddressAndPortAsString(buf);
            spdlog::info("got OSC from {}", buf);

            // Create a hash of command suffix.
            const OscCommandPair* pair = Perfect_Hash::in_word_set(command + kPrefixLength,
                commandLength - kPrefixLength);

            if (!pair) {
                spdlog::error("unsupported OSC command {}", command);
                return;
            }

            switch (pair->number) {
                case kNone:
                    break;

                case kNotify:
                    break;

                case kStatus:
                    break;

                case kQuit:
                    spdlog::info("scinsynth got OSC quit command, terminating.");
                    m_quitSocket.reset(new UdpTransmitSocket(endpoint));
                    m_quitHandler();
                    break;

                case kDumpOSC:
                    break;

                case kError:
                    break;

                case kVersion:
                    break;
            }
        } catch (osc::Exception exception) {
            spdlog::error("exception in processing OSC message");
        }
    }

    void setQuitHandler(std::function<void()> quitHandler) { m_quitHandler = quitHandler; }

    void sendQuitDone() {
        if (m_quitSocket) {
            std::array<char, 32> buffer;
            osc::OutboundPacketStream p(buffer.data(), sizeof(buffer));
            p << osc::BeginMessage("/scin_done") << osc::EndMessage;
            m_quitSocket->Send(p.Data(), p.Size());
        }
    }

private:
    OscHandler* m_handler;

    // Function to call back if a /scin_quit command is received, tells the rest of the scinsynth binary to quit.
    std::function<void()> m_quitHandler;
    // Pointer to the sender to /scin_quit, if non-null, to which we send a /scin_done message right before shutdown.
    std::unique_ptr<UdpTransmitSocket> m_quitSocket;
};

OscHandler::OscHandler(const std::string& bindAddress, int listenPort) :
    m_bindAddress(bindAddress),
    m_listenPort(listenPort) {
}

OscHandler::~OscHandler() {
}

void OscHandler::setQuitHandler(std::function<void()> quitHandler) {
    m_listener->setQuitHandler(quitHandler);
}

void OscHandler::run() {
    m_listener.reset(new OscListener(this));
    m_listenSocket.reset(new UdpListeningReceiveSocket(IpEndpointName(m_bindAddress.data(), m_listenPort),
    m_listener.get()));
    m_socketFuture = std::async(std::launch::async, [this] {
        m_listenSocket->Run();
    });
}

void OscHandler::shutdown() {
    m_listener->sendQuitDone();
    m_listenSocket->AsynchronousBreak();
}

}  // namespace scin

