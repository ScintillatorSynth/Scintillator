#include "vulkan/FrameTimer.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

const size_t kFramePeriodWindowSize = 60;

FrameTimer::FrameTimer(bool trackDroppedFrames):
    m_trackDroppedFrames(trackDroppedFrames),
    m_periodSum(0),
    m_lateFrames(0) {}

FrameTimer::~FrameTimer() {}

void FrameTimer::start() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastFrameTime = m_startTime;
    m_lastReportTime = m_startTime;
}

void FrameTimer::markFrame() {
    TimePoint now(std::chrono::high_resolution_clock::now());
    double framePeriod = std::chrono::duration<double, std::chrono::seconds::period>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;

    double meanPeriod = m_framePeriods.size() ? m_periodSum / static_cast<double>(m_framePeriods.size()) : framePeriod;
    m_periodSum += framePeriod;
    m_framePeriods.push_back(framePeriod);

    // We consider a frame late when we have at least half of the window of frame times to establish a credible
    // mean, and the period of the frame is more than half again the mean.
    if (m_trackDroppedFrames && m_framePeriods.size() >= kFramePeriodWindowSize / 2
        && framePeriod >= (meanPeriod * 1.5)) {
        ++m_lateFrames;
        // Remove the outlier from the average, to avoid biasing our dropped frame detector.
        m_periodSum -= framePeriod;
        m_framePeriods.pop_back();
    }

    while (m_framePeriods.size() > kFramePeriodWindowSize) {
        m_periodSum -= m_framePeriods.front();
        m_framePeriods.pop_front();
    }

    if (std::chrono::duration<double, std::chrono::seconds::period>(now - m_lastReportTime).count() >= 10.0) {
        if (m_trackDroppedFrames) {
            spdlog::info("mean fps: {:.1f}, late frames: {}", 1.0 / meanPeriod, m_lateFrames);
            m_lateFrames = 0;
        } else {
            spdlog::info("mean fps: {:.1f}", 1.0 / meanPeriod);
        }
        m_lastReportTime = now;
    }
}

double FrameTimer::elapsedTime() {
    return std::chrono::duration<double, std::chrono::seconds::period>(m_lastFrameTime - m_startTime).count();
}


} // namespace vk

} // namespace scin
