#include "OscHandler.hpp"

#include "OscCommand.h"

#include "ip/UdpSocket.h"
#include "osc/OscPacketListener.h"
#include "osc/OscReceivedElements.h"

#include <cstring>
#include <future>
#include <stdio.h>

namespace scin {

class OscHandler::OscListener : public osc::OscPacketListener {
public:
    OscListener(OscHandler* handler) :
        osc::OscPacketListener(),
        m_handler(handler) {
    }

    void ProcessMessage(const osc::ReceivedMessage& message, const IpEndpointName& endpoint) override {
        try {
            // Create a hash of the message
            const OscCommandPair* pair = Perfect_Hash::in_word_set(message.AddressPattern(),
                strlen(message.AddressPattern()));
            if (!pair) {
                // unknown message
                fprintf(stderr, "unknown OSC message %s\n", message.AddressPattern());
            } else {
                switch (pair->number) {
                    case kNone:
                        break;

                    case kNotify:
                        break;

                    case kStatus:
                        break;

                    case kQuit:
                        break;

                    case kDumpOSC:
                        break;

                    case kError:
                        break;

                    case kVersion:
                        break;
                }
            }
        } catch (osc::Exception exception) {
        }
    }

private:
    OscHandler* m_handler;
};

OscHandler::OscHandler(int listenPort, const std::string& bindAddress) :
    m_listenPort(listenPort),
    m_bindAddress(bindAddress) {
}

OscHandler::~OscHandler() {
}

void OscHandler::run() {
    m_listener.reset(new OscListener(this));
    m_listenSocket.reset(new UdpListeningReceiveSocket(IpEndpointName(m_bindAddress.data(), m_listenPort),
        m_listener.get()));
    std::async(std::launch::async, [this] {
        m_listenSocket->Run();
    });
}

void OscHandler::shutdown() {
    m_listenSocket->AsynchronousBreak();
}

}  // namespace scin

