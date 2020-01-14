#include "Async.hpp"

#include "Compositor.hpp"
#include "ScinthDef.hpp"
#include "av/CodecChooser.hpp"
#include "core/Archetypes.hpp"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

namespace scin {

Async::Async(std::shared_ptr<core::Archetypes> archetypes, std::shared_ptr<Compositor> compositor,
             std::shared_ptr<av::CodecChooser> codecChooser):
    m_compositor(compositor),
    m_archetypes(archetypes),
    m_codecChooser(codecChooser),
    m_quit(false) {}

Async::~Async() { stop(); }

void Async::run(size_t numberOfWorkerThreads) {
    spdlog::info("Async starting {} worker threads.", numberOfWorkerThreads);
    for (auto i = 0; i < numberOfWorkerThreads; ++i) {
        std::string threadName = fmt::format("asyncWorkerThread_{}", i);
        m_workerThreads.emplace_back(std::thread(&Async::threadMain, this, threadName));
    }
}

void Async::stop() {
    if (!m_quit) {
        m_quit = true;
        m_jobQueueCondition.notify_all();
        for (auto& thread : m_workerThreads) {
            thread.join();
        }
        m_workerThreads.clear();

        // All threads are now terminated, mutex no longer required to access m_jobQueue.
        spdlog::info("Async terminated with {} jobs left in queue.", m_jobQueue.size());
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

void Async::mediaTypeTagQuery(std::string typeTag, std::string mimeType, std::function<void(std::string)> completion) {
    {
        std::lock_guard<std::mutex> lock(m_jobQueueMutex);
        m_jobQueue.emplace_back(
            [this, typeTag, mimeType, completion]() { asyncMediaTypeTagQuery(typeTag, mimeType, completion); });
    }
    m_jobQueueCondition.notify_one();
}

void Async::threadMain(std::string threadName) {
    spdlog::info("Async worker {} starting up.", threadName);

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
                }
            }
        }
    }

    spdlog::info("Async worker {} got termination signal, exiting.", threadName);
}

void Async::asyncVGenLoadDirectory(fs::path path, std::function<void(int)> completion) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        spdlog::error("nonexistent or not directory path {} for VGens.", path.string());
        completion(-1);
        return;
    }

    spdlog::info("Parsing yaml files in {} for AbstractVGens.", path.string());
    auto parseCount = 0;
    for (auto entry : fs::directory_iterator(path)) {
        auto p = entry.path();
        if (fs::is_regular_file(p) && p.extension() == ".yaml") {
            spdlog::debug("Parsing AbstractVGen yaml file {}.", p.string());
            parseCount += m_archetypes->loadAbstractVGensFromFile(p.string());
        }
    }
    spdlog::info("Parsed {} unique VGens.", parseCount);
    completion(parseCount);
}

void Async::asyncScinthDefLoadDirectory(fs::path path, std::function<void(int)> completion) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        spdlog::error("nonexistent or not directory path {} for ScinthDefs.", path.string());
        completion(-1);
        return;
    }

    spdlog::info("Parsing yaml files in directory {} for ScinthDefs.", path.string());
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
    spdlog::info("Parsed {} unique ScinthDefs from directory {}.", parseCount, path.string());
    completion(parseCount);
}

void Async::asyncScinthDefLoadFile(fs::path path, std::function<void(int)> completion) {
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        spdlog::error("nonexistent or nonfile path {} for ScinthDefs.", path.string());
        completion(-1);
        return;
    }
    spdlog::info("Loading ScinthDefs from file {}.", path.string());
    std::vector<std::shared_ptr<const core::AbstractScinthDef>> scinthDefs = m_archetypes->loadFromFile(path.string());
    auto parseCount = 0;
    for (auto scinthDef : scinthDefs) {
        if (m_compositor->buildScinthDef(scinthDef)) {
            ++parseCount;
        }
    }
    spdlog::info("Parsed {} unique ScinthDefs from file {}.", parseCount, path.string());
    completion(parseCount);
}

void Async::asyncScinthDefParseString(std::string yaml, std::function<void(int)> completion) {
    std::vector<std::shared_ptr<const core::AbstractScinthDef>> scinthDefs = m_archetypes->parseFromString(yaml);
    for (auto scinthDef : scinthDefs) {
        m_compositor->buildScinthDef(scinthDef);
    }
    completion(scinthDefs.size());
}

void Async::asyncMediaTypeTagQuery(std::string typeTag, std::string mimeType,
                                   std::function<void(std::string)> completion) {
    std::string tag = m_codecChooser->getTypeTag(typeTag, mimeType);
    completion(tag);
}

} // namespace scin
