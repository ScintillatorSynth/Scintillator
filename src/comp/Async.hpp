#ifndef SRC_COMP_ASYNC_HPP_
#define SRC_COMP_ASYNC_HPP_

#include "base/FileSystem.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace scin {

namespace av {
class CodecChooser;
}

namespace base {
class Archetypes;
}

namespace vk {
class Device;
}

namespace comp {

class Compositor;

/*! Maintains a thread pool and provides facilities to run all async functions on those threads.
 */
class Async {
public:
    Async(std::shared_ptr<base::Archetypes> archetypes, std::shared_ptr<Compositor> compositor,
          std::shared_ptr<vk::Device> device);
    ~Async();

    /*! Nonblocking. Starts the worker threads and the sync thread, then returns.
     *
     * \param numberOfWorkerthreads Number of worker threads to start. Should be > 0.
     */
    void run(size_t numberOfWorkerThreads);

    /*! Adds the callback to a queue to be called the next time all worker threads are idle and the job queue is
     * empty.
     *
     * \param callback A function to call when the Async jobs are complete.
     */
    void sync(std::function<void()> callback);

    /*! Signals all threads to exit, even if there's work remaining in the queue.
     */
    void stop();

    /*! Async load all VGen yaml files at path.
     *
     * \param path The file path to load VGen yaml data from.
     * \param completion The function to call on completion of loading. The completion argument is the number of valid
     *        VGens loaded.
     */
    void vgenLoadDirectory(fs::path path, std::function<void(int)> completion);

    /*! Async load all ScinthDef yaml files at path.
     *
     * \param path The directory path to load ScinthDef yaml data from.
     * \param completion The function to call on completion of loading. The completion argument is the number of valid
     *        ScinthDefs loaded.
     */
    void scinthDefLoadDirectory(fs::path path, std::function<void(int)> completion);

    /*! Async load ScinthDefs from a file.
     *
     * \param path The yaml file path to load ScinthDefs from.
     * \param completion The function to call on completion of loading. The completion argument is the number of valid
     *        ScinthDefs loaded.
     */
    void scinthDefLoadFile(fs::path path, std::function<void(int)> completion);

    /*! Async parse a ScinthDef yaml string.
     *
     * \param yaml The yaml string to parse and load.
     * \param completion The function to call on completion of parsing. The completion argument is the number of valid
     *        ScinthDefs parsed.
     */
    void scinthDefParseString(std::string yaml, std::function<void(int)> completion);

    /*! Puts on of the async worker threads to sleep for the provided number of seconds. Useful for testing.
     *
     * \param seconds The number of seconds to sleep for.
     */
    void sleepFor(int seconds, std::function<void()> completion);

    /*! Async read an image into a newly allocated image buffer.
     *
     * \param bufferID The ID of the newly created buffer. Will clobber any existing buffer with this ID.
     * \param filePath A path to the image file to decode.
     * \param width The requested buffer width, can be -1 to respect original image width.
     * \param height The requested buffer height, can be -1 to respect original image height.
     * \completion The function to call on completion of loading.
     */
    void readImageIntoNewBuffer(int bufferID, std::string filePath, int width, int height,
                                std::function<void()> completion);

private:
    void workerThreadMain(std::string threadName);
    void syncThreadMain();

    void asyncVGenLoadDirectory(fs::path path, std::function<void(int)> completion);

    void asyncScinthDefLoadDirectory(fs::path path, std::function<void(int)> completion);
    void asyncScinthDefLoadFile(fs::path path, std::function<void(int)> completion);
    void asyncScinthDefParseString(std::string yaml, std::function<void(int)> completion);

    void asyncSleepFor(int seconds, std::function<void()> completion);

    void asyncReadImageIntoNewBuffer(int bufferID, std::string filePath, int width, int height,
                                     std::function<void()> completion);

    std::shared_ptr<base::Archetypes> m_archetypes;
    std::shared_ptr<Compositor> m_compositor;
    std::shared_ptr<vk::Device> m_device;
    std::atomic<bool> m_quit;
    std::vector<std::thread> m_workerThreads;
    std::thread m_syncThread;

    // Protects m_jobQueue and m_numberOfActiveWorkers.
    std::mutex m_jobQueueMutex;
    std::condition_variable m_jobQueueCondition;
    std::deque<std::function<void()>> m_jobQueue;
    std::condition_variable m_activeWorkersCondition;
    size_t m_numberOfActiveWorkers;

    // Protects m_syncCallbacks.
    std::mutex m_syncCallbackMutex;
    std::condition_variable m_syncActiveCondition;
    std::deque<std::function<void()>> m_syncCallbacks;

    std::mutex m_stagingMutex;
    std::unordered_set<size_t> m_activeStaging;
    size_t m_stagingSerial;
    std::condition_variable m_activeStagingCondition;
};

} // namespace comp

} // namespace scin

#endif // SRC_COMP_ASYNC_HPP_
