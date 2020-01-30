#include "osc/commands/Status.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

Status::Status(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Status::~Status() {}

void Status::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {
/*
    size_t numberOfWarnings = 0;
    size_t numberOfErrors = 0;
    m_logger->getCounts(numberOfWarnings, numberOfErrors);
    size_t graphicsBytesUsed = 0;
    size_t graphicsBytesAvailable = 0;
    m_compositor->getGraphicsMemoryBudget(graphicsBytesUsed, graphicsBytesAvailable);
    int targetFrameRate = 0;
    double meanFrameRate = 0;
    size_t lateFrames = 0;
    m_frameTimer->getStats(targetFrameRate, meanFrameRate, lateFrames);
    sendMessage(endpoint, "/scin_status.reply", m_compositor->numberOfRunningScinths(), 1,
                static_cast<int32_t>(m_archetypes->numberOfAbstractScinthDefs()),
                static_cast<int32_t>(numberOfWarnings), static_cast<int32_t>(numberOfErrors),
                static_cast<double>(graphicsBytesUsed), static_cast<double>(graphicsBytesAvailable), targetFrameRate,
                meanFrameRate, static_cast<int32_t>(lateFrames));
*/
}

} // namespace commands
} // namespace osc
} // namespace scin
