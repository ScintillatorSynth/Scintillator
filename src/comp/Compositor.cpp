#include "comp/Compositor.hpp"

#include "base/AbstractSampler.hpp"
#include "base/AbstractScinthDef.hpp"
#include "base/VGen.hpp"
#include "comp/AudioStager.hpp"
#include "comp/Canvas.hpp"
#include "comp/ImageMap.hpp"
#include "comp/SamplerFactory.hpp"
#include "comp/Scinth.hpp"
#include "comp/ScinthDef.hpp"
#include "comp/ShaderCompiler.hpp"
#include "comp/StageManager.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"
#include "vulkan/Sampler.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

Compositor::Compositor(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas):
    m_clearColor(0.0f, 0.0f, 0.0f) {}

Compositor::~Compositor() {}


bool Compositor::cue(const std::string& scinthDefName, int nodeID,
                     const std::vector<std::pair<std::string, float>>& namedValues,
                     const std::vector<std::pair<int, float>>& indexedValues) {
}

void Compositor::freeNodes(const std::vector<int>& nodeIDs) {
    std::lock_guard<std::mutex> lock(m_scinthMutex);
    bool needRebuild = false;
    for (auto nodeID : nodeIDs) {
        auto node = m_scinthMap.find(nodeID);
        if (node != m_scinthMap.end()) {
            // Only need to rebuild the command buffers if we are removing running Scinths from the list.
            if ((*(node->second))->running()) {
                needRebuild = true;
            }
            (*(node->second))->destroy();
            freeScinthLockAcquired(node);
        } else {
            spdlog::warn("Compositor attempted to free nonexistent nodeID {}", nodeID);
        }
    }

    if (needRebuild) {
        m_commandBufferDirty = true;
    }
}

void Compositor::groupFreeAll() {
    std::lock_guard<std::mutex> lock(m_scinthMutex);
    m_scinths.clear();
    m_scinthMap.clear();
    m_commandBufferDirty = true;
}

void Compositor::setRun(const std::vector<std::pair<int, int>>& pairs) {
    std::lock_guard<std::mutex> lock(m_scinthMutex);
    for (const auto& pair : pairs) {
        auto node = m_scinthMap.find(pair.first);
        if (node != m_scinthMap.end()) {
            (*(node->second))->setRunning(pair.second != 0);
        } else {
            spdlog::warn("Compositor attempted to set pause/play on nonexistent nodeID {}", pair.first);
        }
    }

    m_commandBufferDirty = true;
}

void Compositor::setNodeParameters(int nodeID, const std::vector<std::pair<std::string, float>>& namedValues,
                                   const std::vector<std::pair<int, float>> indexedValues) {
    std::lock_guard<std::mutex> lock(m_scinthMutex);
    auto nodePair = m_scinthMap.find(nodeID);
    if (nodePair == m_scinthMap.end()) {
        spdlog::warn("Compositor attempted to set parameters on nonexistent nodeID {}", nodeID);
        return;
    }
    auto node = *(nodePair->second);

    for (auto namedPair : namedValues) {
        node->setParameterByName(namedPair.first, namedPair.second);
    }
    for (auto indexedPair : indexedValues) {
        node->setParameterByIndex(indexedPair.first, indexedPair.second);
    }

    m_commandBufferDirty = true;
}

void Compositor::releaseCompiler() { m_shaderCompiler->releaseCompiler(); }

size_t Compositor::numberOfRunningScinths() {
    std::lock_guard<std::mutex> lock(m_scinthMutex);
    return m_scinths.size();
}

bool Compositor::getGraphicsMemoryBudget(size_t& bytesUsedOut, size_t& bytesBudgetOut) {
    return m_device->getGraphicsMemoryBudget(bytesUsedOut, bytesBudgetOut);
}

// Needs to be called only from the same thread that calls prepareFrame. Assumes that m_drawSecondary is up-to-date.
bool Compositor::rebuildCommandBuffer() {
}

void Compositor::freeScinthLockAcquired(ScinthMap::iterator it) {
    // Remove from list first, then dictionary,
    m_scinths.erase(it->second);
    m_scinthMap.erase(it);
}

} // namespace comp

} // namespace scin
