#include "osc/commands/AdvanceFrame.hpp"

#include "osc/Address.hpp"
#include "osc/Dispatcher.hpp"
#include "comp/Offscreen.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace osc { namespace commands {

AdvanceFrame::AdvanceFrame(osc::Dispatcher* dispatcher): Command(dispatcher) {}

AdvanceFrame::~AdvanceFrame() {}

void AdvanceFrame::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (!m_dispatcher->offscreen() || !m_dispatcher->offscreen()->isSnapShotMode()) {
        spdlog::error("Advance Frame requested but scinsynth not in snap shot mode.");
        m_dispatcher->respond(address, "/scin_done", "/scin_nrt_advanceFrame");
        return;
    }
    if (argc < 2 || std::strncmp(types, "ii", 2) != 0) {
        spdlog::error("AdvanceFrame got wrong argument types {}", types);
        m_dispatcher->respond(address, "/scin_done", "/scin_nrt_advanceFrame");
        return;
    }

    int32_t numerator = *reinterpret_cast<int32_t*>(argv[0]);
    int32_t denominator = *reinterpret_cast<int32_t*>(argv[1]);
    double dt = static_cast<double>(numerator) / static_cast<double>(denominator);
    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->offscreen()->advanceFrame(dt, [this, origin](size_t /* frameNumber */) {
        spdlog::info("got callback from advanceFrame");
        m_dispatcher->respond(origin->get(), "/scin_done", "/scin_nrt_advanceFrame");
    });
}

} // namespace commands
} // namespace osc
} // namespace scin
