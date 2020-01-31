#include "osc/commands/Sync.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

Sync::Sync(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Sync::~Sync() {}

void Sync::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {}

} // namespace commands
} // namespace osc
} // namespace scin
