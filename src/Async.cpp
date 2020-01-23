#include "Async.hpp"

#include "Compositor.hpp"
#include "ScinthDef.hpp"
#include "core/Archetypes.hpp"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

namespace scin {

Async::Async(std::shared_ptr<core::Archetypes> archetypes, std::shared_ptr<Compositor> compositor):
    m_compositor(compositor),
    m_archetypes(archetypes),
    m_quit(false),
    m_numberOfActiveWorkers(0) {}

Async::~Async() { stop(); }

void Async::run(size_t numberOfWorkerThreads) {
    size_t workers = std::max(1ul, numberOfWorkerThreads);
    spdlog::info("Async starting {} worker threads.", workers);
    for (auto i = 0; i < workers; ++i) {
        std::string threadName = fmt::format("asyncWorkerThread_{}", i);
        m_workerThreads.emplace_back(std::thread(&Async::workerThreadMain, this, threadName));
    }
    m_syncThread = std::thread(&Async::syncThreadMain, this);
}

void Async::sync(std::function<void()> callback) {
    {
        std::lock_guard<std::mutex> lock(m_syncCallbackMutex);
        m_syncCallbacks.push_back(callback);
    }
    m_syncActiveCondition.notify_one();
}

void Async::stop() {
    if (!m_quit) {
        m_quit = true;
        m_jobQueueCondition.notify_all();
        m_activeWorkersCondition.notify_all();
        for (auto& thread : m_workerThreads) {
            thread.join();
        }
        m_workerThreads.clear();
        m_syncThread.join();

        // All threads are now terminated, mutex no longer required to access m_jobQueue.
        spdlog::debug("Async terminated with {} jobs left in queue.", m_jobQueue.size());
    }
}

void Async::vgenLoadDirectory(fs::path path, std::function<void(int)> completion) {
    {
        std::lock_guard<std::mutex> lock(m_jobQueueMutex);
        m_jobQueue.emplace_back([this, path, completion]() { asyncVGenLoadDirectory(path, completion); });
    }
    m_jobQueueCondition.notify_one();
}

void Async::scinthDefLoadDirectory(fs::path path, std::function<void(int)> completion) {
    {
        std::lock_guard<std::mutex> lock(m_jobQueueMutex);
        m_jobQueue.emplace_back([this, path, completion]() { asyncScinthDefLoadDirectory(path, completion); });
    }
    m_jobQueueCondition.notify_one();
}

void Async::scinthDefLoadFile(fs::path path, std::function<void(int)> completion) {
    {
        std::lock_guard<std::mutex> lock(m_jobQueueMutex);
        m_jobQueue.emplace_back([this, path, completion]() { asyncScinthDefLoadFile(path, completion); });
    }
    m_jobQueueCondition.notify_one();
}

void Async::scinthDefParseString(std::string yaml, std::function<void(int)> completion) {
    {
        std::lock_guard<std::mutex> lock(m_jobQueueMutex);
        m_jobQueue.emplace_back([this, yaml, completion]() { asyncScinthDefParseString(yaml, completion); });
    }
    m_jobQueueCondition.notify_one();
}

void Async::workerThreadMain(std::string threadName) {
    spdlog::debug("Async worker {} starting up.", threadName);

    while (!m_quit) {
        std::function<void()> workFunction;
        bool hasWork = false;

        {
            std::unique_lock<std::mutex> lock(m_jobQueueMutex);
            m_jobQueueCondition.wait(lock, [this] { return m_quit || m_jobQueue.size(); });
            if (m_quit) {
                break;
            }

            if (m_jobQueue.size()) {
                workFunction = m_jobQueue.front();
                m_jobQueue.pop_front();
                hasWork = true;
                ++m_numberOfActiveWorkers;
            }
        }

        // If there's additional work in the queue don't wait on the condition variable, just work until the queue is
        // empty or we get a termination signal.
        while (!m_quit && hasWork) {
            workFunction();
            {
                std::lock_guard<std::mutex> lock(m_jobQueueMutex);
                if (m_jobQueue.size()) {
                    workFunction = m_jobQueue.front();
                    m_jobQueue.pop_front();
                } else {
                    hasWork = false;
                    --m_numberOfActiveWorkers;
                }
            }
        }

        // Exiting the while loop means this thread is about to go dormant, ping the active workers condition check if
        // we were the last ones to go idle.
        m_activeWorkersCondition.notify_one();
    }

    spdlog::debug("Async worker {} got termination signal, exiting.", threadName);
}

void Async::syncThreadMain() {
    spdlog::debug("Async sync watcher thread starting.");

    while (!m_quit) {
        // First we wait for there to be something in the sync callback queue, meaning a sync is requested.
        {
            std::unique_lock<std::mutex> lock(m_syncCallbackMutex);
            m_syncActiveCondition.wait(lock, [this] { return m_quit || m_syncCallbacks.size() > 0; });
            if (m_quit) {
                break;
            }
            if (!m_syncCallbacks.size()) {
                continue;
            }
        }

        spdlog::debug("Async has sync callback, sync watcher thread waiting for idle workers.");

        // Then we wait for the threads to become all idle, and the queue to be empty.
        {
            std::unique_lock<std::mutex> lock(m_jobQueueMutex);
            m_activeWorkersCondition.wait(
                lock, [this] { return m_quit || (m_jobQueue.size() == 0 && m_numberOfActiveWorkers == 0); });
            if (m_quit) {
                break;
            }
            if (m_jobQueue.size() > 0 || m_numberOfActiveWorkers > 0) {
                continue;
            }
        }

        spdlog::debug("Async sync watcher thread idle, firing callbacks.");

        // Now we can empty the callback queue.
        std::function<void()> syncCallback;
        bool validCallback = false;
        {
            std::unique_lock<std::mutex> lock(m_syncCallbackMutex);
            if (m_syncCallbacks.size()) {
                validCallback = true;
                syncCallback = m_syncCallbacks.front();
                m_syncCallbacks.pop_front();
            }
        }

        while (validCallback) {
            syncCallback();
            {
                std::unique_lock<std::mutex> lock(m_syncCallbackMutex);
                if (m_syncCallbacks.size()) {
                    validCallback = true;
                    syncCallback = m_syncCallbacks.front();
                    m_syncCallbacks.pop_front();
                } else {
                    validCallback = false;
                }
            }
        }

        spdlog::debug("Async sync watcher exhausted callbacks.");
    }

    spdlog::debug("Async sync watcher thread exiting.");
}

void Async::asyncVGenLoadDirectory(fs::path path, std::function<void(int)> completion) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        spdlog::error("nonexistent or not directory path {} for VGens.", path.string());
        completion(-1);
        return;
    }

    spdlog::debug("Parsing yaml files in {} for AbstractVGens.", path.string());
    auto parseCount = 0;
    for (auto entry : fs::directory_iterator(path)) {
        auto p = entry.path();
        if (fs::is_regular_file(p) && p.extension() == ".yaml") {
            spdlog::debug("Parsing AbstractVGen yaml file {}.", p.string());
            parseCount += m_archetypes->loadAbstractVGensFromFile(p.string());
        }
    }
    spdlog::debug("Parsed {} unique VGens.", parseCount);
    completion(parseCount);
}

void Async::asyncScinthDefLoadDirectory(fs::path path, std::function<void(int)> completion) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        spdlog::error("nonexistent or not directory path {} for ScinthDefs.", path.string());
        completion(-1);
        return;
    }

    spdlog::debug("Parsing yaml files in directory {} for ScinthDefs.", path.string());
    auto parseCount = 0;
    for (auto entry : fs::directory_iterator(path)) {
        auto p = entry.path();
        if (fs::is_regular_file(p) && p.extension() == ".yaml") {
            spdlog::debug("Parsing ScinthDef yaml file {}.", p.string());
            std::vector<std::shared_ptr<const core::AbstractScinthDef>> scinthDefs =
                m_archetypes->loadFromFile(p.string());
            for (auto scinthDef : scinthDefs) {
                if (m_compositor->buildScinthDef(scinthDef)) {
                    ++parseCount;
                }
            }
        }
    }
    spdlog::debug("Parsed {} unique ScinthDefs from directory {}.", parseCount, path.string());
    completion(parseCount);
}

void Async::asyncScinthDefLoadFile(fs::path path, std::function<void(int)> completion) {
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        spdlog::error("nonexistent or nonfile path {} for ScinthDefs.", path.string());
        completion(-1);
        return;
    }
    spdlog::debug("Loading ScinthDefs from file {}.", path.string());
    std::vector<std::shared_ptr<const core::AbstractScinthDef>> scinthDefs = m_archetypes->loadFromFile(path.string());
    auto parseCount = 0;
    for (auto scinthDef : scinthDefs) {
        if (m_compositor->buildScinthDef(scinthDef)) {
            ++parseCount;
        }
    }
    spdlog::debug("Parsed {} unique ScinthDefs from file {}.", parseCount, path.string());
    completion(parseCount);
}

void Async::asyncScinthDefParseString(std::string yaml, std::function<void(int)> completion) {
    std::vector<std::shared_ptr<const core::AbstractScinthDef>> scinthDefs = m_archetypes->parseFromString(yaml);
    for (auto scinthDef : scinthDefs) {
        m_compositor->buildScinthDef(scinthDef);
    }
    completion(scinthDefs.size());
}

} // namespace scin
