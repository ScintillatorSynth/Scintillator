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
    Async(std::shared_ptr<core::Archetypes> archetypes, std::shared_ptr<Compositor> compositor,
          std::shared_ptr<av::CodecChooser> codecChooser);
    ~Async();

    void run(size_t numberOfWorkerThreads);
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

    /*! Async look up support for a specific media type tag based on tag and/or mime type.
     *
     * \param typeTag The existing type tag, if known, or an empty string.
     * \param mimeType A mime type, if known, or an empty string.
     * \param completion The function to call on completion of query. The completion argument is the media type tag, or
     *        an empty string if type is not found or not supported.
     */
    void mediaTypeTagQuery(std::string typeTag, std::string mimeType, std::function<void(std::string)> completion);

private:
    void threadMain(std::string threadName);

    void asyncVGenLoadDirectory(fs::path path, std::function<void(int)> completion);

    void asyncScinthDefLoadDirectory(fs::path path, std::function<void(int)> completion);
    void asyncScinthDefLoadFile(fs::path path, std::function<void(int)> completion);
    void asyncScinthDefParseString(std::string yaml, std::function<void(int)> completion);

    void asyncMediaTypeTagQuery(std::string typeTag, std::string mimeType, std::function<void(std::string)> completion);

    std::shared_ptr<core::Archetypes> m_archetypes;
    std::shared_ptr<Compositor> m_compositor;
    std::shared_ptr<av::CodecChooser> m_codecChooser;
    std::atomic<bool> m_quit;
    std::vector<std::thread> m_workerThreads;

    std::mutex m_jobQueueMutex;
    std::condition_variable m_jobQueueCondition;
    std::deque<std::function<void()>> m_jobQueue;
};

} // namespace scin

#endif // SRC_ASYNC_HPP_
