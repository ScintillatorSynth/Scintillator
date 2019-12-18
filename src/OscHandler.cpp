#include "OscHandler.hpp"

#include "LogLevels.hpp"
#include "OscCommand.hpp"
#include "Version.hpp"

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "osc/OscPrintReceivedElements.h"
#include "osc/OscReceivedElements.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <cstring>
#include <string>

namespace scin {

class OscHandler::OscListener : public osc::OscPacketListener {
public:
    OscListener(OscHandler* handler):
        osc::OscPacketListener(),
        m_handler(handler),
        m_dumpOSC(false),
        m_quitHandler([] {}) {}

    void ProcessMessage(const osc::ReceivedMessage& message, const IpEndpointName& endpoint) override {
        try {
            if (m_dumpOSC) {
                spdlog::info("dumpOSC: {}", message);
            }

            // All scinsynth messages start with a scin_ prefix, to avoid confusion with similar messages and responses
            // that are sent to scsynth. So we check or this prefix first, and ignore any messages that lack it.
            const char* kPrefix = "/scin_";
            const size_t kPrefixLength = 6;
            const char* command = message.AddressPattern();
            size_t commandLength = std::strlen(command);
            if (commandLength < kPrefixLength || std::strncmp(command, kPrefix, kPrefixLength) != 0) {
                spdlog::error("OSC command {} does not have {} prefix, is not a scinsynth command.", command, kPrefix);
                return;
            }

            // Create a hash of command suffix.
            const OscCommandPair* pair =
                Perfect_Hash::in_word_set(command + kPrefixLength, commandLength - kPrefixLength);

            if (!pair) {
                spdlog::error("unsupported OSC command {}", command);
                return;
            }

            // couple of needs that we have here. There will be some low-latency messages that should probably not be a
            // delegated to a std::async task, but rather should be handled directly on this thread. Or maybe that is
            // a premature optimization. Normal thing will be to collect a std::future from a std::async launch to
            // to handle the response. What happens to those futures? For lightweight things, like setting the quit
            // flag, doing on a separate thread seems excessive.
            //
            // Separately, there should be some way to send a reply back to the sender. Again here there are two use
            // cases, the first where some code here is executing so the context is available to send the response. The
            // other case is when some other thread has to do work, in which case maybe completion callbacks should be
            // the thing, and they will encapsulate the necessary information to send the thing back.
            switch (pair->number) {
            case kNone:
                // None command means this is deliberately empty.
                break;

            case kNotify:
                // TBD the server having something to notify about.
                break;

            case kStatus:
                // TBD meaningful status.
                break;

            case kQuit:
                spdlog::info("scinsynth got OSC quit command, terminating.");
                m_quitSocket.reset(new UdpTransmitSocket(endpoint));
                m_quitHandler();
                break;

            case kDumpOSC: {
                osc::ReceivedMessage::const_iterator msg = message.ArgumentsBegin();
                int dump = (msg++)->AsInt32();
                if (msg != message.ArgumentsEnd())
                    throw osc::ExcessArgumentException();
                if (dump == 0) {
                    spdlog::info("Turning off OSC message dumping to the log.");
                    m_dumpOSC = false;
                } else {
                    spdlog::info("Dumping parsed OSC messages to the log.");
                    m_dumpOSC = true;
                }
            } break;

            case kLogLevel: {
                osc::ReceivedMessage::const_iterator msg = message.ArgumentsBegin();
                int logLevel = (msg++)->AsInt32();
                if (msg != message.ArgumentsEnd())
                    throw osc::ExcessArgumentException();
                spdlog::info("Setting log level to {}.", logLevel);
                setGlobalLogLevel(logLevel);
            } break;

            case kVersion: {
                UdpTransmitSocket responseSocket(endpoint);
                std::array<char, 1024> buffer;
                osc::OutboundPacketStream p(buffer.data(), sizeof(buffer));
                p << osc::BeginMessage("/scin_version.reply");
                p << "scinsynth";
                p << kScinVersionMajor;
                p << kScinVersionMinor;
                p << kScinVersionPatch;
                p << kScinBranch;
                p << kScinCommitHash;
                p << osc::EndMessage;
                responseSocket.Send(p.Data(), p.Size());
            } break;
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
    bool m_dumpOSC;

    // Function to call back if a /scin_quit command is received, tells the rest of the scinsynth binary to quit.
    std::function<void()> m_quitHandler;
    // Pointer to the sender to /scin_quit, if non-null, to which we send a /scin_done message right before shutdown.
    std::unique_ptr<UdpTransmitSocket> m_quitSocket;
};

OscHandler::OscHandler(const std::string& bindAddress, int listenPort):
    m_bindAddress(bindAddress),
    m_listenPort(listenPort) {}

OscHandler::~OscHandler() {}

void OscHandler::setQuitHandler(std::function<void()> quitHandler) { m_listener->setQuitHandler(quitHandler); }

void OscHandler::run() {
    m_listener.reset(new OscListener(this));
    m_listenSocket.reset(
        new UdpListeningReceiveSocket(IpEndpointName(m_bindAddress.data(), m_listenPort), m_listener.get()));
    m_socketFuture = std::async(std::launch::async, [this] { m_listenSocket->Run(); });
}

void OscHandler::shutdown() {
    m_listener->sendQuitDone();
    m_listenSocket->AsynchronousBreak();
}

} // namespace scin
