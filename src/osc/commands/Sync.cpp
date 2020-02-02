#include "osc/commands/Sync.hpp"

#include "Async.hpp"
#include "osc/Address.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace osc { namespace commands {

Sync::Sync(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Sync::~Sync() {}

void Sync::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc < 1 || types[0] != LO_INT32) {
        spdlog::error("OSC Sync got missing or non-integer argument");
        return;
    }
    int32_t id = *reinterpret_cast<int32_t*>(argv[0]);
    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->async()->sync([this, id, origin]() { m_dispatcher->respond(origin->get(), "/scin_synced", id); });
}

} // namespace commands
} // namespace osc
} // namespace scin
