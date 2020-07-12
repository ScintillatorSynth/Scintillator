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
    m_device(device),
    m_canvas(canvas),
    m_clearColor(0.0f, 0.0f, 0.0f),
    m_shaderCompiler(new ShaderCompiler()),
    m_computeCommandPool(new vk::CommandPool(device)),
    m_drawCommandPool(new vk::CommandPool(device)),
    m_stageManager(new StageManager(device)),
    m_samplerFactory(new SamplerFactory(device)),
    m_imageMap(new ImageMap()),
    m_commandBufferDirty(true),
    m_nodeSerial(-2) {
    m_computeCommands.resize(m_canvas->numberOfImages());
    m_drawCommands.resize(m_canvas->numberOfImages());
}

Compositor::~Compositor() {}

bool Compositor::create() {
    if (!m_shaderCompiler->loadCompiler()) {
        spdlog::error("Compositor unable to load shader compiler.");
        return false;
    }

    if (!m_computeCommandPool->create()) {
        spdlog::error("Compositor failed creating compute command pool.");
    }

    if (!m_drawCommandPool->create()) {
        spdlog::error("Compositor failed creating draw command pool.");
        return false;
    }

    if (!m_stageManager->create(m_canvas->numberOfImages())) {
        spdlog::error("Compositor failed to create stage manager.");
        return false;
    }

    // Create the empty image and stage.
    std::shared_ptr<vk::HostBuffer> emptyImageBuffer(new vk::HostBuffer(m_device, vk::Buffer::Kind::kStaging, 4));
    if (!emptyImageBuffer->create()) {
        spdlog::error("Compositor failed to create empty staging image.");
        return false;
    }
    std::memset(emptyImageBuffer->mappedAddress(), 0, 4);

    std::shared_ptr<vk::DeviceImage> emptyImage(new vk::DeviceImage(m_device, VK_FORMAT_R8G8B8A8_UNORM));
    if (!emptyImage->create(1, 1)) {
        spdlog::error("Compositor failed to create empty device image.");
        return false;
    }

    if (!m_stageManager->stageImage(emptyImageBuffer, emptyImage, [this, emptyImage] {
            if (!emptyImage->createView()) {
                spdlog::error("Compositor failed to create ImageView for empty image");
                return;
            }
            m_imageMap->setEmptyImage(emptyImage);
            spdlog::info("Compositor finished staging the empty image");
        })) {
        spdlog::error("Compositor failed to stage the empty image.");
        return false;
    }

    rebuildCommandBuffer();
    return true;
}

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

bool Compositor::prepareFrame(uint32_t imageIndex, double frameTime) {
    m_computeSecondary.clear();
    m_drawSecondary.clear();

    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        for (auto stager : m_audioStagers) {
            stager->stageAudio(m_stageManager);
        }

        for (auto scinth : m_scinths) {
            if (scinth->running()) {
                scinth->prepareFrame(imageIndex, frameTime);
                std::shared_ptr<vk::CommandBuffer> computeCommands = scinth->computeCommands();
                if (computeCommands) {
                    computeCommands->associateScinth(scinth);
                    m_computeSecondary.emplace_back(computeCommands);
                }
                std::shared_ptr<vk::CommandBuffer> drawCommands = scinth->drawCommands();
                drawCommands->associateScinth(scinth);
                m_drawSecondary.emplace_back(drawCommands);
            }
        }
    }

    m_computeCommands[imageIndex] = m_computeSecondary;
    m_drawCommands[imageIndex] = m_drawSecondary;

    if (m_commandBufferDirty) {
        rebuildCommandBuffer();
    }

    return m_drawPrimary != nullptr;
}

void Compositor::releaseCompiler() { m_shaderCompiler->releaseCompiler(); }

void Compositor::destroy() {
    // Delete all command buffers outstanding first, which means emptying the Scinth map and list.
    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        m_scinthMap.clear();
        for (auto scinth : m_scinths) {
            scinth->destroy();
        }
        m_scinths.clear();
        m_audioStagers.clear();
    }
    // TODO: as a last resort, could call destroy on each of these
    m_drawPrimary.reset();
    m_drawSecondary.clear();
    m_drawCommands.clear();

    // We leave the command pools undestroyed as there may be outstanding commandbuffers pipelined. The shared_ptr
    // system should collect them before all is done. But we must remove our references to them or we keep them alive
    // until our own destructor.
    m_computeCommandPool = nullptr;
    m_drawCommandPool = nullptr;

    m_stageManager->destroy();
    m_imageMap = nullptr;
    m_samplerFactory = nullptr;

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

bool Compositor::queryImage(int imageID, int& sizeOut, int& widthOut, int& heightOut) {
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
    if (m_computeSecondary.size()) {
        m_computePrimary.reset(new vk::CommandBuffer(m_device, m_computeCommandPool));
        if (!m_computePrimary->create(m_canvas->numberOfImages(), true)) {
            spdlog::critical("failed creating primary compute command buffers for Compositor.");
            return false;
        }

        m_computePrimary->associateSecondaryCommands(m_computeSecondary);
        spdlog::debug("rebuilding Compositor compute command buffer with {} secondary command buffers",
                m_computeSecondary.size());

        for (auto i = 0; i < m_canvas->numberOfImages(); ++i) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(m_computePrimary->buffer(i), &beginInfo) != VK_SUCCESS) {
                spdlog::error("Compositor failed beginning primary compute command buffer.");
                return false;
            }

            std::vector<VkCommandBuffer> commandBuffers;
            for (auto command : m_computeSecondary) {
                commandBuffers.emplace_back(command->buffer(i));
            }
            vkCmdExecuteCommands(m_computePrimary->buffer(i), commandBuffers.size(), commandBuffers.data());

            if (vkEndCommandBuffer(m_computePrimary->buffer(i)) != VK_SUCCESS) {
                spdlog::error("Compositor failed ending primary compute command buffer.");
                return false;
            }
        }
    } else {
        m_computePrimary = nullptr;
    }

    m_drawPrimary.reset(new vk::CommandBuffer(m_device, m_drawCommandPool));
    if (!m_drawPrimary->create(m_canvas->numberOfImages(), true)) {
        spdlog::critical("failed creating primary draw command buffers for Compositor.");
        return false;
    }

    // Ensure the secondary command buffers get retained as long as this buffer is retained.
    m_drawPrimary->associateSecondaryCommands(m_drawSecondary);

    VkClearColorValue clearColor = { { m_clearColor.x, m_clearColor.y, m_clearColor.z, 1.0f } };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    spdlog::debug("rebuilding Compositor draw command buffer with {} secondary command buffers",
                  m_drawSecondary.size());

    for (auto i = 0; i < m_canvas->numberOfImages(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(m_drawPrimary->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::error("Compositor failed beginning primary draw command buffer.");
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

        if (m_drawSecondary.size()) {
            vkCmdBeginRenderPass(m_drawPrimary->buffer(i), &renderPassInfo,
                                 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

            std::vector<VkCommandBuffer> commandBuffers;
            for (auto command : m_drawSecondary) {
                commandBuffers.emplace_back(command->buffer(i));
            }
            vkCmdExecuteCommands(m_drawPrimary->buffer(i), commandBuffers.size(), commandBuffers.data());
        } else {
            vkCmdBeginRenderPass(m_drawPrimary->buffer(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        vkCmdEndRenderPass(m_drawPrimary->buffer(i));
        if (vkEndCommandBuffer(m_drawPrimary->buffer(i)) != VK_SUCCESS) {
            spdlog::error("Compositor failed ending primary draw command buffer.");
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
