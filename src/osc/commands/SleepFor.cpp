#include "osc/commands/SleepFor.hpp"

#include "comp/Async.hpp"
#include "osc/Address.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <memory>

namespace scin { namespace osc { namespace commands {

SleepFor::SleepFor(osc::Dispatcher* dispatcher): Command(dispatcher) {}

SleepFor::~SleepFor() {}

void SleepFor::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc < 1 || types[0] != LO_INT32) {
        spdlog::error("OSC SleepFor got absent or non-integer argument.");
        m_dispatcher->respond(address, "/scin_awake");
        return;
    }
    int32_t seconds = *reinterpret_cast<int32_t*>(argv[0]);
    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->async()->sleepFor(seconds, [this, origin]() { m_dispatcher->respond(origin->get(), "/scin_awake"); });
}

} // namespace commands
} // namespace osc
} // namespace scin
