#include "comp/FrameTimer.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

const size_t kFramePeriodWindowSize = 60;

FrameTimer::FrameTimer(int targetFrameRate):
    m_trackLateFrames(targetFrameRate < 0),
    m_periodSum(0),
    m_lateFrames(0),
    m_targetFrameRate(targetFrameRate),
    m_meanFrameRate(0),
    m_totalLateFrames(0) {}

FrameTimer::~FrameTimer() {}

void FrameTimer::start() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastFrameTime = m_startTime;
    m_lastUpdateTime = m_startTime;
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
    if (m_trackLateFrames && m_framePeriods.size() >= kFramePeriodWindowSize / 2 && framePeriod >= (meanPeriod * 1.5)) {
        ++m_lateFrames;
        // Remove the outlier from the average, to avoid biasing our dropped frame detector.
        m_periodSum -= framePeriod;
        m_framePeriods.pop_back();
    }

    while (m_framePeriods.size() > kFramePeriodWindowSize) {
        m_periodSum -= m_framePeriods.front();
        m_framePeriods.pop_front();
    }

    if (std::chrono::duration<double, std::chrono::seconds::period>(now - m_lastUpdateTime).count() >= 10.0) {
        updateStats(meanPeriod);
        m_lastUpdateTime = now;
    }
}

double FrameTimer::elapsedTime() {
    return std::chrono::duration<double, std::chrono::seconds::period>(m_lastFrameTime - m_startTime).count();
}

void FrameTimer::getStats(int& targetFrameRateOut, double& meanFrameRateOut, size_t& lateFramesOut) const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    targetFrameRateOut = m_targetFrameRate;
    meanFrameRateOut = m_meanFrameRate;
    lateFramesOut = m_totalLateFrames;
}

void FrameTimer::updateStats(double meanPeriod) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    if (meanPeriod > 0) {
        m_meanFrameRate = 1.0 / meanPeriod;
    } else {
        m_meanFrameRate = 0.0;
    }
    if (m_targetFrameRate < 0) {
        spdlog::info("mean fps: {:.1f}, late frames: {}", m_meanFrameRate, m_lateFrames);
        m_lateFrames = 0;
    } else {
        spdlog::info("mean fps: {:.1f}", m_meanFrameRate);
    }

    m_totalLateFrames += m_lateFrames;
    m_lateFrames = 0;
}

} // namespace comp

} // namespace scin
