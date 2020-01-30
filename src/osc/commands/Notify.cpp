#include "osc/commands/Notify.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

Notify::Notify(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Notify::~Notify() {}

void Notify::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {
    // TBD rest of notification system.
}

} // namespace commands
} // namespace osc
} // namespace scin
