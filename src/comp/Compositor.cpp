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


bool Compositor::buildScinthDef(std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef) {
    std::shared_ptr<ScinthDef> scinthDef(new ScinthDef(m_device, m_canvas, m_computeCommandPool, m_drawCommandPool,
                                                       m_samplerFactory, abstractScinthDef));
    if (!scinthDef->build(m_shaderCompiler.get())) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_scinthDefMutex);
        m_scinthDefs.insert_or_assign(abstractScinthDef->name(), std::move(scinthDef));
    }

    return true;
}

void Compositor::freeScinthDefs(const std::vector<std::string>& names) {
    std::lock_guard<std::mutex> lock(m_scinthDefMutex);
    for (auto name : names) {
        auto it = m_scinthDefs.find(name);
        if (it != m_scinthDefs.end()) {
            m_scinthDefs.erase(it);
        } else {
            spdlog::warn("unable to free ScinthDef {}, name not found.", name);
        }
    }
}

bool Compositor::cue(const std::string& scinthDefName, int nodeID,
                     const std::vector<std::pair<std::string, float>>& namedValues,
                     const std::vector<std::pair<int, float>>& indexedValues) {
    std::shared_ptr<ScinthDef> scinthDef;
    {
        std::lock_guard<std::mutex> lock(m_scinthDefMutex);
        auto it = m_scinthDefs.find(scinthDefName);
        if (it != m_scinthDefs.end()) {
            scinthDef = it->second;
        }
    }
    if (!scinthDef) {
        spdlog::error("ScinthDef {} not found when building Scinth {}.", scinthDefName, nodeID);
        return false;
    }

    // Generate a unique negative nodeID if the one provided was negative.
    if (nodeID < 0) {
        nodeID = m_nodeSerial.fetch_sub(1);
    }
    std::shared_ptr<Scinth> scinth(new Scinth(m_device, nodeID, scinthDef, m_imageMap));

    if (!scinth->create()) {
        spdlog::error("failed to build Scinth {} from ScinthDef {}.", nodeID, scinthDefName);
        return false;
    }

    // Override default values in the Scinth as required.
    for (const auto& pair : namedValues) {
        scinth->setParameterByName(pair.first, pair.second);
    }
    for (const auto& pair : indexedValues) {
        scinth->setParameterByIndex(pair.first, pair.second);
    }

    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        auto oldNode = m_scinthMap.find(nodeID);
        if (oldNode != m_scinthMap.end()) {
            spdlog::info("clobbering existing Scinth {}", nodeID);
            freeScinthLockAcquired(oldNode);
        }
        m_scinths.push_back(scinth);
        auto it = m_scinths.end();
        --it;
        m_scinthMap.insert({ nodeID, it });
    }

    spdlog::info("Scinth id {} from def {} cueued.", nodeID, scinthDefName);

    // Will need to rebuild command buffer on next frame to include the new scinths.
    m_commandBufferDirty = true;
    return true;
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

void Compositor::stageImage(int imageID, uint32_t width, uint32_t height, std::shared_ptr<vk::HostBuffer> imageBuffer,
                            std::function<void()> completion) {
    std::shared_ptr<vk::DeviceImage> targetImage(new vk::DeviceImage(m_device, VK_FORMAT_R8G8B8A8_UNORM));
    if (!targetImage->create(width, height)) {
        spdlog::error("Compositor failed to create staging target image {}.", imageID);
        completion();
        return;
    }

    if (!m_stageManager->stageImage(imageBuffer, targetImage, [this, imageID, targetImage, completion] {
            if (!targetImage->createView()) {
                spdlog::error("Compositor failed to create ImageView for image {}", imageID);
                completion();
                return;
            }
            m_imageMap->addImage(imageID, targetImage);
            spdlog::info("Compositor finished staging image id {}", imageID);
            completion();
        })) {
        spdlog::error("Compositor encountered error while staging image {}.", imageID);
        completion();
    }
}

bool Compositor::queryImage(int imageID, size_t& sizeOut, uint32_t& widthOut, uint32_t& heightOut) {
    std::shared_ptr<vk::DeviceImage> image = m_imageMap->getImage(imageID);
    if (!image) {
        return false;
    }
    sizeOut = image->size();
    widthOut = image->width();
    heightOut = image->height();
    return true;
}

bool Compositor::addAudioIngress(std::shared_ptr<audio::Ingress> ingress, int imageID) {
    std::shared_ptr<AudioStager> stager(new AudioStager(ingress));
    if (!stager->createBuffers(m_device)) {
        spdlog::error("Compositor failed to create AudioStager buffers.");
        return false;
    }
    m_imageMap->addImage(imageID, stager->image());

    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        m_audioStagers.push_back(stager);
    }
    return true;
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
