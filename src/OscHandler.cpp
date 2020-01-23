#include "OscHandler.hpp"

#include "Async.hpp"
#include "Compositor.hpp"
#include "Logger.hpp"
#include "OscCommand.hpp"
#include "Version.hpp"
#include "av/ImageEncoder.hpp"
#include "core/Archetypes.hpp"
#include "vulkan/FrameTimer.hpp"
#include "vulkan/Offscreen.hpp"

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "osc/OscPrintReceivedElements.h"
#include "osc/OscReceivedElements.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

namespace scin {

class OscHandler::OscListener : public osc::OscPacketListener {
public:
    OscListener(std::shared_ptr<Logger> logger, std::shared_ptr<Async> async,
                std::shared_ptr<core::Archetypes> archetypes, std::shared_ptr<Compositor> compositor,
                std::shared_ptr<vk::Offscreen> offscreen, std::shared_ptr<const vk::FrameTimer> frameTimer,
                std::function<void()> quitHandler):
        osc::OscPacketListener(),
        m_logger(logger),
        m_async(async),
        m_archetypes(archetypes),
        m_compositor(compositor),
        m_offscreen(offscreen),
        m_frameTimer(frameTimer),
        m_quitHandler(quitHandler),
        m_encodersSerial(0),
        m_sendQuitDone(false),
        m_dumpOSC(false) {}

    // TODO: fixup callback types and response messages for the parsing/loading functions.
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

            case kStatus: {
                UdpTransmitSocket responseSocket(endpoint);
                std::array<char, 1024> buffer;
                osc::OutboundPacketStream p(buffer.data(), sizeof(buffer));
                p << osc::BeginMessage("/scin_status.reply");
                p << m_compositor->numberOfRunningScinths();
                p << 1; // Number of groups currently always 1.
                p << static_cast<int>(m_archetypes->numberOfAbstractScinthDefs());
                size_t numberOfWarnings = 0;
                size_t numberOfErrors = 0;
                m_logger->getCounts(numberOfWarnings, numberOfErrors);
                p << static_cast<int32_t>(numberOfWarnings);
                p << static_cast<int32_t>(numberOfErrors);
                size_t graphicsBytesUsed = 0;
                size_t graphicsBytesAvailable = 0;
                m_compositor->getGraphicsMemoryBudget(graphicsBytesUsed, graphicsBytesAvailable);
                p << static_cast<double>(graphicsBytesUsed);
                p << static_cast<double>(graphicsBytesAvailable);
                int targetFrameRate = 0;
                double meanFrameRate = 0;
                size_t lateFrames = 0;
                m_frameTimer->getStats(targetFrameRate, meanFrameRate, lateFrames);
                p << targetFrameRate;
                p << meanFrameRate;
                p << static_cast<int32_t>(lateFrames);
                p << osc::EndMessage;
                responseSocket.Send(p.Data(), p.Size());
            } break;

            case kQuit:
                spdlog::info("scinsynth got OSC quit command, terminating.");
                m_quitEndpoint = endpoint;
                m_sendQuitDone = true;
                m_quitHandler();
                break;

            case kDumpOSC: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                int dump = (args++)->AsInt32();
                if (dump == 0) {
                    spdlog::info("Turning off OSC message dumping to the log.");
                    m_dumpOSC = false;
                } else {
                    spdlog::info("Dumping parsed OSC messages to the log.");
                    m_dumpOSC = true;
                }
            } break;

            case kSync: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                int syncId = (args++)->AsInt32();
                m_async->sync([syncId, endpoint]() {
                    UdpTransmitSocket responseSocket(endpoint);
                    std::array<char, 64> buffer;
                    osc::OutboundPacketStream p(buffer.data(), sizeof(buffer));
                    p << osc::BeginMessage("/scin_synced");
                    p << syncId;
                    p << osc::EndMessage;
                    responseSocket.Send(p.Data(), p.Size());
                });
            } break;

            case kLogLevel: {
                osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                int logLevel = (args++)->AsInt32();
                spdlog::info("Setting log level to {}.", logLevel);
                m_logger->setConsoleLogLevel(logLevel);
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
                m_compositor->cue(scinthDef, nodeID);
            } break;

            case kNRTScreenShot: {
                if (m_offscreen) {
                    osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                    std::string fileName = (args++)->AsString();
                    std::string mimeType;
                    if (args != message.ArgumentsEnd() && args->IsString()) {
                        mimeType = (args++)->AsString();
                    }
                    int serial = m_encodersSerial.fetch_add(1);
                    std::shared_ptr<av::ImageEncoder> imageEncoder(new av::ImageEncoder(
                        m_offscreen->width(), m_offscreen->height(), [this, endpoint, fileName, serial](bool valid) {
                            spdlog::info("Screenshot finished encode of '{}', valid: {}", fileName, valid);
                            std::array<char, 128> buffer;
                            UdpTransmitSocket socket(endpoint);
                            osc::OutboundPacketStream p(buffer.data(), sizeof(buffer));
                            p << osc::BeginMessage("/scin_done") << "/scin_nrt_screenShot" << fileName.data();
                            p << osc::EndMessage;
                            socket.Send(p.Data(), p.Size());
                            {
                                std::lock_guard<std::mutex> lock(m_encodersMutex);
                                m_encoders.erase(serial);
                            }
                        }));
                    if (imageEncoder->createFile(fileName, mimeType)) {
                        spdlog::info("Screenshot '{}' enqueued for encode.", fileName);
                        m_offscreen->addEncoder(imageEncoder);
                        {
                            std::lock_guard<std::mutex> lock(m_encodersMutex);
                            m_encoders.insert(std::make_pair(serial, imageEncoder));
                        }
                    } else {
                        spdlog::error("Screenshot failed to create file '{}' with mimeType '{}'", fileName, mimeType);
                    }
                } else {
                    spdlog::error("Screenshot requested but scinsynth is not running in non-realtime.");
                }
            } break;

            case kNRTAdvanceFrame: {
                if (m_offscreen && m_offscreen->isSnapShotMode()) {
                    osc::ReceivedMessage::const_iterator args = message.ArgumentsBegin();
                    int numerator = (args++)->AsInt32();
                    int denominator = (args++)->AsInt32();
                    double dt = static_cast<double>(numerator) / static_cast<double>(denominator);
                    m_offscreen->advanceFrame(dt, [this, endpoint](size_t frameNumber) {
                        std::array<char, 128> buffer;
                        UdpTransmitSocket socket(endpoint);
                        osc::OutboundPacketStream p(buffer.data(), sizeof(buffer));
                        p << osc::BeginMessage("/scin_done") << "/scin_nrt_advanceFrame";
                        p << static_cast<int32_t>(frameNumber);
                        p << osc::EndMessage;
                        socket.Send(p.Data(), p.Size());
                    });
                } else {
                    spdlog::error("Advance Frame requested but scinsynth not in snap shot mode.");
                }
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
    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<Async> m_async;
    std::shared_ptr<core::Archetypes> m_archetypes;
    std::shared_ptr<Compositor> m_compositor;
    std::shared_ptr<vk::Offscreen> m_offscreen;
    std::shared_ptr<const vk::FrameTimer> m_frameTimer;
    std::function<void()> m_quitHandler;
    std::atomic<int> m_encodersSerial;
    std::mutex m_encodersMutex;
    std::unordered_map<int, std::shared_ptr<av::Encoder>> m_encoders;
    bool m_sendQuitDone;
    bool m_dumpOSC;
    IpEndpointName m_quitEndpoint;
};

OscHandler::OscHandler(const std::string& bindAddress, int listenPort):
    m_bindAddress(bindAddress),
    m_listenPort(listenPort) {}

OscHandler::~OscHandler() {}

bool OscHandler::run(std::shared_ptr<Logger> logger, std::shared_ptr<Async> async,
                     std::shared_ptr<core::Archetypes> archetypes, std::shared_ptr<Compositor> compositor,
                     std::shared_ptr<vk::Offscreen> offscreen, std::shared_ptr<const vk::FrameTimer> frameTimer,
                     std::function<void()> quitHandler) {
    m_listener.reset(new OscListener(logger, async, archetypes, compositor, offscreen, frameTimer, quitHandler));
    try {
        m_listenSocket.reset(
            new UdpListeningReceiveSocket(IpEndpointName(m_bindAddress.data(), m_listenPort), m_listener.get()));
    } catch (std::runtime_error e) {
        spdlog::error("caught exception opening socket: {}", e.what());
        return false;
    }
    m_socketThread = std::thread([this] { m_listenSocket->Run(); });
    return true;
}

void OscHandler::shutdown() {
    m_listener->sendQuitDone();
    m_listenSocket->AsynchronousBreak();
    m_socketThread.join();
}

} // namespace scin
