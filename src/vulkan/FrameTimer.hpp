#ifndef SRC_VULKAN_FRAME_TIMER_HPP_
#define SRC_VULKAN_FRAME_TIMER_HPP_

#include <chrono>
#include <deque>

namespace scin { namespace vk {

/*! Used to track average throughput and latency of frame rendering.
 */
class FrameTimer {
public:
    FrameTimer(bool trackDroppedFrames);
    ~FrameTimer();

    /*! Saves the start time and starts tracking frame-to-frame distances.
     */
    void start();

    /*! Mark a frame as started, updates internal timing statistics, and may generate a report in the log.
     */
    void markFrame();

    /*! Returns the time elapsed in seconds from the call to start() to the most recent call to markFrame().
     *
     * \return elapsed time in seconds.
     */
    double elapsedTime();

private:
    bool m_trackDroppedFrames;

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
    std::deque<double> m_framePeriods;
    double m_periodSum;
    TimePoint m_startTime;
    TimePoint m_lastFrameTime;
    size_t m_lateFrames;
    TimePoint m_lastReportTime;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_FRAME_TIMER_HPP_
