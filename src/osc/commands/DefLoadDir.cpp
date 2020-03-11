#include "osc/commands/DefLoadDir.hpp"

#include "comp/Async.hpp"
#include "osc/Address.hpp"
#include "osc/BlobMessage.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <algorithm>

namespace scin { namespace osc { namespace commands {

DefLoadDir::DefLoadDir(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DefLoadDir::~DefLoadDir() {}

void DefLoadDir::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc < 1 || types[0] != LO_STRING) {
        spdlog::error("OSC DefLoadDir missing/incorrect directory path string argument.");
        m_dispatcher->respond(address, "/scin_done");
        return;
    }
    std::string path(reinterpret_cast<const char*>(argv[0]));
    std::shared_ptr<BlobMessage> onCompletion;
    if (argc > 1 && types[1] == LO_BLOB) {
        onCompletion.reset(new BlobMessage());
        if (!onCompletion->extract(reinterpret_cast<lo_blob>(argv[1]))) {
            spdlog::error("OSC DefLoadDir failed to extract blob message.");
            m_dispatcher->respond(address, "/scin_done");
            return;
        }
    }
    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->async()->scinthDefLoadDirectory(path, [this, origin, onCompletion](int) {
        if (onCompletion) {
            m_dispatcher->processMessageFrom(origin->get(), onCompletion);
        }
        m_dispatcher->respond(origin->get(), "/scin_done");
    });
}

} // namespace commands
} // namespace osc
} // namespace scin
