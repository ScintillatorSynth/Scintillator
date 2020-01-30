#include "osc/commands/ScinthNew.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

ScinthNew::ScinthNew(osc::Dispatcher* dispatcher): Command(dispatcher) {}

ScinthNew::~ScinthNew() {}

void ScinthNew::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
