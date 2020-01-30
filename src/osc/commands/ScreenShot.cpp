#include "osc/commands/ScreenShot.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

ScreenShot::ScreenShot(osc::Dispatcher* dispatcher): Command(dispatcher) {}

ScreenShot::~ScreenShot() {}

void ScreenShot::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
