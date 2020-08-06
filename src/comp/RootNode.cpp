#include "comp/RootNode.hpp"

#include "audio/Ingress.hpp"
#include "base/AbstractScinthDef.hpp"
#include "comp/AudioStager.hpp"
#include "comp/Canvas.hpp"
#include "comp/FrameContext.hpp"
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

#include <spdlog/spdlog.h>

#include <stack>

namespace scin { namespace comp {

RootNode::RootNode(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas):
    Node(device, 0),
    m_canvas(canvas),
    m_shaderCompiler(new ShaderCompiler()),
    m_computeCommandPool(new vk::CommandPool(device)),
    m_drawCommandPool(new vk::CommandPool(device)),
    m_stageManager(new StageManager(device)),
    m_samplerFactory(new SamplerFactory(device)),
    m_imageMap(new ImageMap()),
    m_commandBufferDirty(true),
    m_nodeSerial(-2) {}

bool RootNode::create() {
    if (!m_shaderCompiler->loadCompiler()) {
        spdlog::critical("RootNode unable to load shader compiler.");
        return false;
    }

    if (!m_computeCommandPool->create()) {
        spdlog::critical("RootNode failed creating compute command pool.");
    }

    if (!m_drawCommandPool->create()) {
        spdlog::critical("RootNode failed creating draw command pool.");
        return false;
    }

    if (!m_stageManager->create(m_canvas->numberOfImages())) {
        spdlog::critical("RootNode failed to create stage manager.");
        return false;
    }

    // Create the empty image and stage.
    std::shared_ptr<vk::HostBuffer> emptyImageBuffer(new vk::HostBuffer(m_device, vk::Buffer::Kind::kStaging, 4));
    if (!emptyImageBuffer->create()) {
        spdlog::critical("RootNode failed to create empty staging image.");
        return false;
    }
    std::memset(emptyImageBuffer->mappedAddress(), 0, 4);

    std::shared_ptr<vk::DeviceImage> emptyImage(new vk::DeviceImage(m_device, VK_FORMAT_R8G8B8A8_UNORM));
    if (!emptyImage->create(1, 1)) {
        spdlog::critical("RootNode failed to create empty device image.");
        return false;
    }

    if (!m_stageManager->stageImage(emptyImageBuffer, emptyImage, [this, emptyImage] {
            if (!emptyImage->createView()) {
                spdlog::critical("RootNode failed to create ImageView for empty image");
                return false;
            }
            m_imageMap->setEmptyImage(emptyImage);
            spdlog::debug("RootNode finished staging the empty image");
            return true;
        })) {
        spdlog::critical("RootNode failed to stage the empty image.");
        return false;
    }

    return true;
}

bool RootNode::prepareFrame(std::shared_ptr<FrameContext> context) {
    {
        std::lock_guard<std::mutex> lock(m_nodeMutex);
        for (auto stager : m_audioStagers) {
            stager->stageAudio(m_stageManager);
        }

        for (auto child : m_children) {
            context->appendNode(child);
            m_commandBufferDirty |= child->prepareFrame(context);
        }
    }

    bool rebuildRequired = m_commandBufferDirty;
    if (rebuildRequired) {
        rebuildCommandBuffer(context);
    }

    context->setComputePrimary(m_computePrimary);
    context->setDrawPrimary(m_drawPrimary);

    return rebuildRequired;
}

void RootNode::setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                             const std::vector<std::pair<int, float>>& indexedValues) {
    // Note it is assumed the node lock has already been acquired.
    for (auto child : m_children) {
        child->setParameters(namedValues, indexedValues);
    }
}

void RootNode::destroy() {
    // Delete all command buffers outstanding first, which means emptying the Scinth map and list.
    {
        std::lock_guard<std::mutex> lock(m_nodeMutex);
        m_nodeMap.clear();
        m_children.clear();
        m_audioStagers.clear();
    }
    m_computePrimary.reset();
    m_drawPrimary.reset();

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

bool RootNode::buildScinthDef(std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef) {
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

void RootNode::freeScinthDefs(const std::vector<std::string>& names) {
    std::lock_guard<std::mutex> lock(m_scinthDefMutex);
    for (auto name : names) {
        auto it = m_scinthDefs.find(name);
        if (it != m_scinthDefs.end()) {
            m_scinthDefs.erase(it);
        } else {
            spdlog::warn("Unable to free ScinthDef {}, name not found.", name);
        }
    }
}

void RootNode::stageImage(int imageID, uint32_t width, uint32_t height, std::shared_ptr<vk::HostBuffer> imageBuffer,
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

bool RootNode::queryImage(int imageID, size_t& sizeOut, uint32_t& widthOut, uint32_t& heightOut) {
    std::shared_ptr<vk::DeviceImage> image = m_imageMap->getImage(imageID);
    if (!image) {
        return false;
    }
    sizeOut = image->size();
    widthOut = image->width();
    heightOut = image->height();
    return true;
}

bool RootNode::addAudioIngress(std::shared_ptr<audio::Ingress> ingress, int imageID) {
    std::shared_ptr<AudioStager> stager(new AudioStager(ingress));
    if (!stager->createBuffers(m_device)) {
        spdlog::error("Compositor failed to create AudioStager buffers.");
        return false;
    }
    m_imageMap->addImage(imageID, stager->image());

    {
        std::lock_guard<std::mutex> lock(m_nodeMutex);
        m_audioStagers.push_back(stager);
    }
    return true;
}

void RootNode::scinthNew(const std::string& scinthDefName, int nodeID, AddAction addAction, int targetID,
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
        return;
    }

    // Generate a unique negative nodeID if the one provided was negative.
    if (nodeID < 0) {
        nodeID = m_nodeSerial.fetch_sub(1);
    }
    std::shared_ptr<Scinth> scinth(new Scinth(m_device, nodeID, scinthDef, m_imageMap));

    if (!scinth->create()) {
        spdlog::error("failed to build Scinth {} from ScinthDef {}.", nodeID, scinthDefName);
        return;
    }

    scinth->setParameters(namedValues, indexedValues);

    {
        std::lock_guard<std::mutex> lock(m_nodeMutex);
        if (!insertNode(scinth, addAction, targetID)) {
            spdlog::error("Failed to add Scinth into render tree.");
            return;
        }
    }

    spdlog::info("Scinth id {} from def {} cueued.", nodeID, scinthDefName);

    // Will need to rebuild command buffer on next frame to include the new scinths.
    m_commandBufferDirty = true;
}

void RootNode::nodeFree(const std::vector<int>& nodeIDs) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    for (auto node : nodeIDs) {
        removeNode(node);
    }
}

void RootNode::setNodeParameters(int nodeID, const std::vector<std::pair<std::string, float>>& namedValues,
        const std::vector<std::pair<int, float>>& indexedValues) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    if (nodeID != 0) {
        Node* node;
        auto mapIter = m_nodeMap.find(nodeID);
        if (mapIter == m_nodeMap.end()) {
            spdlog::error("setNodeParameters called on on unknown node ID {}", nodeID);
            return;
        }
        node = mapIter->second->get();
        node->setParameters(namedValues, indexedValues);
    } else {
        setParameters(namedValues, indexedValues);
    }
}

void RootNode::setNodeRun(const std::vector<std::pair<int, int>>& pairs) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    for (const auto& pair : pairs) {
        auto mapIter = m_nodeMap.find(pair.first);
        if (mapIter != m_nodeMap.end()) {
            mapIter->second->get()->setRunning(pair.second != 0);
        } else {
            spdlog::warn("Compositor attempted to set pause/play on nonexistent nodeID {}", pair.first);
        }
    }

    m_commandBufferDirty = true;
}

size_t RootNode::numberOfRunningNodes() {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    return m_nodeMap.size();
}

void RootNode::rebuildCommandBuffer(std::shared_ptr<FrameContext> context) {
    if (context->computeCommands().size()) {
        m_computePrimary.reset(new vk::CommandBuffer(m_device, m_computeCommandPool));
        if (!m_computePrimary->create(m_canvas->numberOfImages(), true)) {
            spdlog::critical("failed creating primary compute command buffers for Compositor.");
            return;
        }

        spdlog::debug("rebuilding Compositor compute command buffer with {} secondary command buffers",
                      context->computeCommands().size());

        for (size_t i = 0; i < m_canvas->numberOfImages(); ++i) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(m_computePrimary->buffer(i), &beginInfo) != VK_SUCCESS) {
                spdlog::critical("Compositor failed beginning primary compute command buffer.");
                return;
            }

            std::vector<VkCommandBuffer> commandBuffers;
            for (const auto command : context->computeCommands()) {
                commandBuffers.emplace_back(command->buffer(i));
            }
            vkCmdExecuteCommands(m_computePrimary->buffer(i), static_cast<uint32_t>(commandBuffers.size()),
                                 commandBuffers.data());

            if (vkEndCommandBuffer(m_computePrimary->buffer(i)) != VK_SUCCESS) {
                spdlog::critical("Compositor failed ending primary compute command buffer.");
                return;
            }
        }
    } else {
        m_computePrimary = nullptr;
    }

    m_drawPrimary.reset(new vk::CommandBuffer(m_device, m_drawCommandPool));
    if (!m_drawPrimary->create(m_canvas->numberOfImages(), true)) {
        spdlog::critical("failed creating primary draw command buffers for Compositor.");
        return;
    }

    // VkClearColorValue clearColor = { { m_clearColor.x, m_clearColor.y, m_clearColor.z, 1.0f } };
    VkClearColorValue clearColor = { { 0.0, 0.0, 0.0, 1.0f } };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    spdlog::debug("rebuilding Compositor draw command buffer with {} secondary command buffers",
                  context->drawCommands().size());

    for (size_t i = 0; i < m_canvas->numberOfImages(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(m_drawPrimary->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::critical("Compositor failed beginning primary draw command buffer.");
            return;
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_canvas->renderPass();
        renderPassInfo.framebuffer = m_canvas->framebuffer(i);
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_canvas->extent();
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        if (context->drawCommands().size()) {
            vkCmdBeginRenderPass(m_drawPrimary->buffer(i), &renderPassInfo,
                                 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

            std::vector<VkCommandBuffer> commandBuffers;
            for (const auto command : context->drawCommands()) {
                commandBuffers.emplace_back(command->buffer(i));
            }
            vkCmdExecuteCommands(m_drawPrimary->buffer(i), static_cast<uint32_t>(commandBuffers.size()),
                                 commandBuffers.data());
        } else {
            vkCmdBeginRenderPass(m_drawPrimary->buffer(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        vkCmdEndRenderPass(m_drawPrimary->buffer(i));
        if (vkEndCommandBuffer(m_drawPrimary->buffer(i)) != VK_SUCCESS) {
            spdlog::critical("Compositor failed ending primary draw command buffer.");
            return;
        }
    }

    m_commandBufferDirty = false;
}

bool RootNode::insertNode(std::shared_ptr<Node> node, AddAction addAction, int targetID) {
    if (targetID != 0) {
        Node* target;
        auto mapIter = m_nodeMap.find(targetID);
        if (mapIter != m_nodeMap.end()) {
            target = mapIter->second->get();
        } else {
            spdlog::error("failed to find node targetID {} on node insert", targetID);
            return false;
        }

        switch (targetID) {
        case kGroupHead: {
            node->setParent(target);
            target->children().emplace_front(node);
            m_nodeMap[node->nodeID()] = target->children().begin();
        } break;

        case kGroupTail: {
            node->setParent(target);
            target->children().emplace_back(node);
            auto lastElement = target->children().end();
            --lastElement;
            m_nodeMap[node->nodeID()] = lastElement;
        } break;

        // Replace does the same as insert before, and then removes the target node after insertion of the new node.
        case kReplace:
        case kBeforeNode: {
            node->setParent(target->parent());
            m_nodeMap[node->nodeID()] = node->parent()->children().emplace(mapIter->second, node);
        } break;

        case kAfterNode: {
            node->setParent(target->parent());
            auto afterIter = mapIter->second;
            ++afterIter;
            m_nodeMap[node->nodeID()] = node->parent()->children().emplace(afterIter, node);
        } break;
        }

        if (addAction == kReplace) {
            removeNode(targetID);
        }
    } else {
        node->setParent(this);

        switch (targetID) {
        case kGroupHead: {
            m_children.emplace_front(node);
            m_nodeMap[node->nodeID()] = m_children.begin();
        } break;

        case kGroupTail: {
            m_children.emplace_back(node);
            auto endIter = m_children.end();
            --endIter;
            m_nodeMap[node->nodeID()] = endIter;
        } break;

        case kBeforeNode:
        case kAfterNode:
        case kReplace:
            spdlog::error("Scinth AddAction {} not supported on root node.", static_cast<int>(addAction));
            return false;
        }
    }

    return true;
}

void RootNode::removeNode(int targetID) {
    if (targetID == 0) {
        spdlog::error("Can't remove root node from render tree.");
        return;
    }

    auto mapIter = m_nodeMap.find(targetID);
    if (mapIter == m_nodeMap.end()) {
        spdlog::error("failed to find node ID {} on node removal", targetID);
        return;
    }

    // Now remove node and all contained nodes from the node map.
    std::stack<Node*> removeNodes;
    removeNodes.push(mapIter->second->get());
    while (removeNodes.size()) {
        Node* node = removeNodes.top();
        removeNodes.pop();
        m_nodeMap.erase(node->nodeID());
        for (auto child : node->children()) {
            removeNodes.push(child.get());
        }
    }

    // Remove node from parent's child list.
    mapIter->second->get()->parent()->children().erase(mapIter->second);
}

} // namespace comp
} // namespace scin
