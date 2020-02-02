#include "osc/commands/DefLoad.hpp"

#include "Async.hpp"
#include "osc/Address.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <algorithm>

namespace scin { namespace osc { namespace commands {

DefLoad::DefLoad(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DefLoad::~DefLoad() {}

void DefLoad::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc < 1 || types[0] != LO_STRING) {
        spdlog::error("OSC DefLoad missing/incorrect file path string argument.");
        m_dispatcher->respond(address, "/scin_done");
        return;
    }
    std::string path(reinterpret_cast<const char*>(argv[0]));
    std::shared_ptr<uint8_t[]> onCompletion;
    uint32_t completionMessageSize = 0;
    if (argc > 1 && types[1] == LO_BLOB) {
        lo_blob blob = reinterpret_cast<lo_blob>(argv[1]);
        completionMessageSize = std::min(lo_blob_datasize(blob), static_cast<uint32_t>(LO_MAX_MSG_SIZE));
        if (completionMessageSize > 0) {
            onCompletion.reset(new uint8_t[completionMessageSize]);
            std::memcpy(onCompletion.get(), lo_blob_dataptr(blob), completionMessageSize);
        }
    }
    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->async()->scinthDefLoadFile(path, [this, origin, completionMessageSize, onCompletion](int) {
        if (onCompletion) {
            m_dispatcher->processMessageFrom(origin->get(), onCompletion, completionMessageSize);
        }
        m_dispatcher->respond(origin->get(), "/scin_done");
    });
}

} // namespace commands
} // namespace osc
} // namespace scin
