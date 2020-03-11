#ifndef SRC_COMP_FRAME_TIMER_HPP_
#define SRC_COMP_FRAME_TIMER_HPP_

#include <chrono>
#include <deque>
#include <mutex>

namespace scin { namespace comp {

/*! Used to track average throughput and latency of frame rendering.
 */
class FrameTimer {
public:
    FrameTimer(int targetFrameRate);
    ~FrameTimer();

    /*! Saves the start time and starts tracking frame-to-frame distances.
     */
    void start();

    /*! Mark a frame as started, updates internal timing statistics, and may generate a report in the log.
     *  Not thread-safe, assumed to be called from the running update loop only.
     */
    void markFrame();

    /*! Returns the time elapsed in seconds from the call to start() to the most recent call to markFrame().
     *  Not thread-safe, assumed to be called from the running update loop only.
     *
     * \return elapsed time in seconds.
     */
    double elapsedTime();

    /*! Returns most recent statistics about timing. Thread-safe, callable from anywhere.
     */
    void getStats(int& targetFrameRateOut, double& meanFrameRateOut, size_t lateFramesOut) const;

private:
    void updateStats(double meanPeriod);

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
    std::deque<double> m_framePeriods;
    bool m_trackLateFrames;
    double m_periodSum;
    TimePoint m_startTime;
    TimePoint m_lastFrameTime;
    size_t m_lateFrames;
    TimePoint m_lastUpdateTime;

    mutable std::mutex m_statsMutex;
    int m_targetFrameRate;
    double m_meanFrameRate;
    size_t m_totalLateFrames;
};

} // namespace comp

} // namespace scin

#endif // SRC_COMP_FRAME_TIMER_HPP_
