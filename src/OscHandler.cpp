#include "OscHandler.hpp"

#include "Async.hpp"
#include "Compositor.hpp"
#include "OscCommand.hpp"
#include "Version.hpp"
#include "core/Archetypes.hpp"
#include "core/LogLevels.hpp"

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
    OscListener(std::shared_ptr<Async> async, std::shared_ptr<core::Archetypes> archetypes,
                std::shared_ptr<Compositor> compositor, std::function<void()> quitHandler):
        osc::OscPacketListener(),
        m_async(async),
        m_archetypes(archetypes),
        m_compositor(compositor),
        m_quitHandler(quitHandler),
        m_sendQuitDone(false),
        m_dumpOSC(false) {}

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
                m_quitEndpoint = endpoint;
                m_sendQuitDone = true;
                m_quitHandler();
                break;

            case kDumpOSC: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                int dump = (args++)->AsInt32();
                if (args != message.ArgumentsEnd())
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
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                int logLevel = (args++)->AsInt32();
                if (args != message.ArgumentsEnd())
                    throw osc::ExcessArgumentException();
                spdlog::info("Setting log level to {}.", logLevel);
                core::setGlobalLogLevel(logLevel);
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

            case kDRecv: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                std::string yaml = (args++)->AsString();
                int completionMessageSize;
                std::shared_ptr<char[]> onCompletion = extractMessage(message, args, completionMessageSize);
                m_async->scinthDefParseString(yaml, [this, onCompletion, completionMessageSize, endpoint](bool) {
                    if (onCompletion) {
                        ProcessPacket(onCompletion.get(), completionMessageSize, endpoint);
                    }
                    sendDoneMessage(endpoint);
                });
            } break;

            case kDLoad: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                std::string path = (args++)->AsString();
                int completionMessageSize;
                std::shared_ptr<char[]> onCompletion = extractMessage(message, args, completionMessageSize);
                m_async->scinthDefLoadFile(path, [this, onCompletion, completionMessageSize, endpoint](bool) {
                    if (onCompletion) {
                        ProcessPacket(onCompletion.get(), completionMessageSize, endpoint);
                    }
                    sendDoneMessage(endpoint);
                });
            } break;

            case kDLoadDir: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                std::string path = (args++)->AsString();
                int completionMessageSize;
                std::shared_ptr<char[]> onCompletion = extractMessage(message, args, completionMessageSize);
                m_async->scinthDefLoadDirectory(path, [this, onCompletion, completionMessageSize, endpoint](bool) {
                    if (onCompletion) {
                        ProcessPacket(onCompletion.get(), completionMessageSize, endpoint);
                    }
                    sendDoneMessage(endpoint);
                });
            } break;

            case kDFree: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                std::vector<std::string> names;
                while (args != message.ArgumentsEnd()) {
                    names.emplace_back((args++)->AsString());
                }
                m_archetypes->freeAbstractScinthDefs(names);
                m_compositor->freeScinthDefs(names);
            } break;

            case kNFree: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                std::vector<int> nodeIDs;
                while (args != message.ArgumentsEnd()) {
                    nodeIDs.emplace_back((args++)->AsInt32());
                }
                m_compositor->freeNodes(nodeIDs);
            } break;

            case kNRun: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                std::vector<std::pair<int, int>> pairs;
                while (args != message.ArgumentsEnd()) {
                    pairs.emplace_back(std::make_pair((args++)->AsInt32(), (args++)->AsInt32()));
                }
                m_compositor->setRun(pairs);
            } break;

            case kSNew: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                std::string scinthDef = (args++)->AsString();
                int nodeID = (args++)->AsInt32();
                // TODO: handle rest of message in terms of placement and group support.
                m_compositor->play(scinthDef, nodeID, std::chrono::high_resolution_clock::now());
            } break;
            }
        } catch (osc::Exception e) {
            spdlog::error("exception in processing OSC message: {}", e.what());
        }
    }

    void sendDoneMessage(const IpEndpointName& endpoint) {
        std::array<char, 32> buffer;
        UdpTransmitSocket socket(endpoint);
        osc::OutboundPacketStream p(buffer.data(), sizeof(buffer));
        p << osc::BeginMessage("/scin_done") << osc::EndMessage;
        socket.Send(p.Data(), p.Size());
    }

    void sendQuitDone() {
        if (m_sendQuitDone) {
            sendDoneMessage(m_quitEndpoint);
        }
    }

    std::shared_ptr<char[]> extractMessage(const osc::ReceivedMessage& message,
                                           const osc::ReceivedMessage::const_iterator& args, int& messageSize) {
        std::shared_ptr<char[]> extracted;
        const void* messageData = nullptr;
        messageSize = 0;
        if (args != message.ArgumentsEnd() && args->IsBlob()) {
            args->AsBlob(messageData, messageSize);
            extracted.reset(new char[messageSize]);
            std::memcpy(extracted.get(), messageData, messageSize);
        }
        return extracted;
    }

private:
    std::shared_ptr<Async> m_async;
    std::shared_ptr<core::Archetypes> m_archetypes;
    std::shared_ptr<Compositor> m_compositor;
    std::function<void()> m_quitHandler;
    bool m_sendQuitDone;
    bool m_dumpOSC;
    IpEndpointName m_quitEndpoint;
};

OscHandler::OscHandler(const std::string& bindAddress, int listenPort):
    m_bindAddress(bindAddress),
    m_listenPort(listenPort) {}

OscHandler::~OscHandler() {}

void OscHandler::run(std::shared_ptr<Async> async, std::shared_ptr<core::Archetypes> archetypes,
                     std::shared_ptr<Compositor> compositor, std::function<void()> quitHandler) {
    m_listener.reset(new OscListener(async, archetypes, compositor, quitHandler));
    m_listenSocket.reset(
        new UdpListeningReceiveSocket(IpEndpointName(m_bindAddress.data(), m_listenPort), m_listener.get()));
    m_socketThread = std::thread([this] { m_listenSocket->Run(); });
}

void OscHandler::shutdown() {
    m_listener->sendQuitDone();
    m_listenSocket->AsynchronousBreak();
    m_socketThread.join();
}

} // namespace scin
