#include "osc/commands/AdvanceFrame.hpp"

#include "osc/Dispatcher.hpp"
#include "vulkan/Offscreen.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace osc { namespace commands {

AdvanceFrame::AdvanceFrame(osc::Dispatcher* dispatcher): Command(dispatcher) {}

AdvanceFrame::~AdvanceFrame() {}

void AdvanceFrame::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {
    if (!m_dispatcher->offscreen() || !m_dispatcher->offscreen()->isSnapShotMode()) {
        spdlog::error("Advance Frame requested but scinsynth not in snap shot mode.");
        return;
    }
    if (std::strncmp(types, "ii", 2) != 0) {
        spdlog::error("AdvanceFrame got wrong argument types {}", types);
        return;
    }
    int32_t numerator = *reinterpret_cast<int32_t*>(argv[0]);
    int32_t denominator = *reinterpret_cast<int32_t*>(argv[1]);
    double dt = static_cast<double>(numerator) / static_cast<double>(denominator);
    lo_address address = lo_message_get_source(message);
    m_dispatcher->offscreen()->advanceFrame(dt, [this, address](size_t frameNumber) {
     //   sendMessage(address, "/scin_done", "/scin_nrt_advanceFrame");
    });
}

} // namespace commands
} // namespace osc
} // namespace scin
