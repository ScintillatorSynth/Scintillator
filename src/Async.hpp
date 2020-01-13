#ifndef SRC_ASYNC_HPP_
#define SRC_ASYNC_HPP_

#include "core/FileSystem.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <deque>
#include <string>
#include <thread>
#include <mutex>
#include <vector>

namespace scin {

namespace av {
class CodecChooser;
}

namespace core {
class Archetypes;
}

class Compositor;

/*! Maintains a thread pool and provides facilities to run all async functions on those threads.
 */
class Async {
public:
    Async(std::shared_ptr<core::Archetypes> archetypes, std::shared_ptr<Compositor> compositor);
    ~Async();

    void run(size_t numberOfWorkerThreads);
    void stop();

    /*! Async load all VGen yaml files at path.
     *
     * \param path The file path to load VGen yaml data from.
     * \param completion The function to call on completion of loading.
     */
    void vgenLoadDirectory(fs::path path, std::function<void(bool)> completion);

    /*! Async load all ScinthDef yaml files at path.
     *
     * \param path The directory path to load ScinthDef yaml data from.
     * \param completion The function to call on completion of loading.
     */
    void scinthDefLoadDirectory(fs::path path, std::function<void(bool)> completion);

    /*! Async load ScinthDefs from a file.
     *
     * \param path The yaml file path to load ScinthDefs from.
     * \param completion The function to call on completion of loading.
     */
    void scinthDefLoadFile(fs::path path, std::function<void(bool)> completion);

    /*! Async parse a ScinthDef yaml string.
     *
     * \param yaml The yaml string to parse and load.
     * \param completion The function to call on completion of parsing.
     */
    void scinthDefParseString(std::string yaml, std::function<void(bool)> completion);

private:
    void threadMain(std::string threadName);

    bool asyncVGenLoadDirectory(fs::path path);

    bool asyncScinthDefLoadDirectory(fs::path path);
    bool asyncScinthDefLoadFile(fs::path path);
    bool asyncScinthDefParseString(std::string yaml);

    std::shared_ptr<core::Archetypes> m_archetypes;
    std::shared_ptr<Compositor> m_compositor;
    std::atomic<bool> m_quit;
    std::vector<std::thread> m_workerThreads;
    std::mutex m_jobQueueMutex;
    std::condition_variable m_jobQueueCondition;
    std::deque<std::pair<std::function<bool()>, std::function<void(bool)>>> m_jobQueue;
};

} // namespace scin

#endif // SRC_ASYNC_HPP_
