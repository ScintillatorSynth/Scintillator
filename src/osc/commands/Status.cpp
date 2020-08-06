#include "osc/commands/Status.hpp"

#include "base/Archetypes.hpp"
#include "comp/RootNode.hpp"
#include "infra/Logger.hpp"
#include "osc/Dispatcher.hpp"
#include "comp/FrameTimer.hpp"

namespace scin { namespace osc { namespace commands {

Status::Status(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Status::~Status() {}

void Status::processMessage(int /* argc */, lo_arg** /* argv */, const char* /* types */, lo_address address) {
    size_t numberOfWarnings = 0;
    size_t numberOfErrors = 0;
    size_t graphicsBytesUsed = 0;
    size_t graphicsBytesAvailable = 0;
    int targetFrameRate = 0;
    double meanFrameRate = 0;
    size_t lateFrames = 0;
    m_dispatcher->logger()->getCounts(numberOfWarnings, numberOfErrors);
    m_dispatcher->frameTimer()->getGraphicsMemoryBudget(graphicsBytesUsed, graphicsBytesAvailable);
    m_dispatcher->frameTimer()->getStats(targetFrameRate, meanFrameRate, lateFrames);
    m_dispatcher->respond(address, "/scin_status.reply",
                          static_cast<int32_t>(m_dispatcher->rootNode()->numberOfRunningNodes()), 1,
                          static_cast<int32_t>(m_dispatcher->archetypes()->numberOfAbstractScinthDefs()),
                          static_cast<int32_t>(numberOfWarnings), static_cast<int32_t>(numberOfErrors),
                          static_cast<double>(graphicsBytesUsed), static_cast<double>(graphicsBytesAvailable),
                          targetFrameRate, meanFrameRate, static_cast<int32_t>(lateFrames));
}

} // namespace commands
} // namespace osc
} // namespace scin
