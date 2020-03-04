#include "comp/Compositor.hpp"

#include "base/AbstractSampler.hpp"
#include "base/AbstractScinthDef.hpp"
#include "base/VGen.hpp"
#include "comp/Canvas.hpp"
#include "comp/SamplerFactory.hpp"
#include "comp/Scinth.hpp"
#include "comp/ScinthDef.hpp"
#include "comp/ShaderCompiler.hpp"
#include "comp/StageManager.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"
#include "vulkan/Sampler.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

Compositor::Compositor(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas):
    m_device(device),
    m_canvas(canvas),
    m_clearColor(0.0f, 0.0f, 0.0f),
    m_shaderCompiler(new ShaderCompiler()),
    m_commandPool(new vk::CommandPool(device)),
    m_stageManager(new StageManager(device)),
    m_samplerFactory(new SamplerFactory(device)),
    m_commandBufferDirty(true),
    m_nodeSerial(-2) {
    m_frameCommands.resize(m_canvas->numberOfImages());
}

Compositor::~Compositor() {}

bool Compositor::create() {
    if (!m_shaderCompiler->loadCompiler()) {
        spdlog::error("Compositor unable to load shader compiler.");
        return false;
    }

    if (!m_commandPool->create()) {
        spdlog::error("Compositor failed creating command pool.");
        return false;
    }

    if (!m_stageManager->create()) {
        spdlog::error("Compositor failed to create stage manager.");
        return false;
    }

    rebuildCommandBuffer();
    return true;
}

bool Compositor::buildScinthDef(std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef) {
    std::shared_ptr<ScinthDef> scinthDef(new ScinthDef(m_device, m_canvas, m_commandPool, abstractScinthDef));
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

bool Compositor::cue(const std::string& scinthDefName, int nodeID) {
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
    std::shared_ptr<Scinth> scinth(new Scinth(m_device, nodeID, scinthDef));

    if (!scinth->create()) {
        spdlog::error("failed to build Scinth {} from ScinthDef {}.", nodeID, scinthDefName);
        return false;
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
            freeScinthLockAcquired(node);
        } else {
            spdlog::warn("Compositor attempted to free nonexistent nodeID {}", nodeID);
        }
    }

    if (needRebuild) {
        m_commandBufferDirty = true;
    }
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

std::shared_ptr<vk::CommandBuffer> Compositor::prepareFrame(uint32_t imageIndex, double frameTime) {
    m_secondaryCommands.clear();

    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        for (auto scinth : m_scinths) {
            if (scinth->running()) {
                scinth->prepareFrame(imageIndex, frameTime);
                m_secondaryCommands.emplace_back(scinth->frameCommands());
            }
        }
    }

    m_frameCommands[imageIndex] = m_secondaryCommands;

    if (m_commandBufferDirty) {
        rebuildCommandBuffer();
    }
    return m_primaryCommands;
}

void Compositor::releaseCompiler() { m_shaderCompiler->releaseCompiler(); }

void Compositor::destroy() {
    // Delete all command buffers outstanding first, which means emptying the Scinth map and list.
    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        m_scinthMap.clear();
        m_scinths.clear();
    }
    m_primaryCommands.reset();
    m_secondaryCommands.clear();
    m_frameCommands.clear();

    // We leave the commandPool undestroyed as there may be outstanding commandbuffers pipelined. The shared_ptr
    // system should collect them before all is done. But we must remove our reference to it or we keep it alive
    // until our own destructor.
    m_commandPool = nullptr;

    // Now delete all of the ScinthDefs, which hold shared graphics resources.
    {
        std::lock_guard<std::mutex> lock(m_scinthDefMutex);
        m_scinthDefs.clear();
    }
}


int Compositor::numberOfRunningScinths() {
    std::lock_guard<std::mutex> lock(m_scinthMutex);
    return m_scinths.size();
}

bool Compositor::getGraphicsMemoryBudget(size_t& bytesUsedOut, size_t& bytesBudgetOut) {
    return m_device->getGraphicsMemoryBudget(bytesUsedOut, bytesBudgetOut);
}

void Compositor::stageImage(int imageID, int width, int height, std::shared_ptr<vk::HostBuffer> imageBuffer,
                            std::function<void()> completion) {
    std::shared_ptr<vk::DeviceImage> targetImage(new vk::DeviceImage(m_device));
    if (!targetImage->create(width, height)) {
        spdlog::error("Compositor failed to create staging target image {}.", imageID);
        completion();
        return;
    }

    if (!m_stageManager->stageImage(imageBuffer, targetImage, [this, imageID, targetImage, completion] {
            {
                std::lock_guard<std::mutex> lock(m_imageMutex);
                m_images[imageID] = targetImage;
            }
            completion();
        })) {
        spdlog::error("Compositor encountered error while staging image {}.", imageID);
        completion();
    }
}

bool Compositor::queryImage(int imageID, int& sizeOut, int& widthOut, int& heightOut) {
    std::lock_guard<std::mutex> lock(m_imageMutex);
    auto it = m_images.find(imageID);
    if (it == m_images.end()) {
        return false;
    }
    sizeOut = it->second->size();
    widthOut = it->second->width();
    heightOut = it->second->height();
    return true;
}

// Needs to be called only from the same thread that calls prepareFrame. Assumes that m_secondaryCommands is up-to-date.
bool Compositor::rebuildCommandBuffer() {
    m_primaryCommands.reset(new vk::CommandBuffer(m_device, m_commandPool));
    if (!m_primaryCommands->create(m_canvas->numberOfImages(), true)) {
        spdlog::critical("failed creating primary command buffers for Compositor.");
        return false;
    }

    VkClearColorValue clearColor = { { m_clearColor.x, m_clearColor.y, m_clearColor.z, 1.0f } };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    spdlog::debug("rebuilding Compositor command buffer with {} secondary command buffers", m_secondaryCommands.size());

    for (auto i = 0; i < m_canvas->numberOfImages(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(m_primaryCommands->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::error("Compositor failed beginning primary command buffer.");
            return false;
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_canvas->renderPass();
        renderPassInfo.framebuffer = m_canvas->framebuffer(i);
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_canvas->extent();
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        if (m_secondaryCommands.size()) {
            vkCmdBeginRenderPass(m_primaryCommands->buffer(i), &renderPassInfo,
                                 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

            std::vector<VkCommandBuffer> commandBuffers;
            for (auto command : m_secondaryCommands) {
                commandBuffers.push_back(command->buffer(i));
            }
            vkCmdExecuteCommands(m_primaryCommands->buffer(i), commandBuffers.size(), commandBuffers.data());
        } else {
            vkCmdBeginRenderPass(m_primaryCommands->buffer(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        vkCmdEndRenderPass(m_primaryCommands->buffer(i));
        if (vkEndCommandBuffer(m_primaryCommands->buffer(i)) != VK_SUCCESS) {
            spdlog::error("Compositor failed ending primary command buffer.");
            return false;
        }
    }

    m_commandBufferDirty = false;
    return true;
}

void Compositor::freeScinthLockAcquired(ScinthMap::iterator it) {
    // Remove from list first, then dictionary,
    m_scinths.erase(it->second);
    m_scinthMap.erase(it);
}

} // namespace comp

} // namespace scin
