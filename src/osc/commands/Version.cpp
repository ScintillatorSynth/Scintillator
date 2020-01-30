#include "osc/commands/Version.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

Version::Version(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Version::~Version() {}

void Version::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
