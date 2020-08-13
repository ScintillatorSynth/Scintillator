#include "comp/RootNode.hpp"

#include "audio/Ingress.hpp"
#include "base/AbstractScinthDef.hpp"
#include "comp/AudioStager.hpp"
#include "comp/Canvas.hpp"
#include "comp/FrameContext.hpp"
#include "comp/Group.hpp"
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
    m_device(device),
    m_canvas(canvas),
    m_shaderCompiler(new ShaderCompiler()),
    m_computeCommandPool(new vk::CommandPool(device)),
    m_drawCommandPool(new vk::CommandPool(device)),
    m_stageManager(new StageManager(device)),
    m_samplerFactory(new SamplerFactory(device)),
    m_imageMap(new ImageMap()),
    m_commandBuffersDirty(true),
    m_nodeSerial(-2),
    m_scinthCount(0),
    m_groupCount(1),
    m_root(new Group(device, 0)) {
    // Root of the tree is a group with ID 0, so set up that association now.
    m_nodes[0] = m_root;
}

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
    bool rebuildRequired = m_commandBuffersDirty;

    {
        std::lock_guard<std::mutex> lock(m_treeMutex);
        for (auto stager : m_audioStagers) {
            stager->stageAudio(m_stageManager);
        }

        rebuildRequired |= m_root->prepareFrame(context);
    }

    if (rebuildRequired) {
        rebuildCommandBuffer(context);
    }

    context->setComputePrimary(m_computePrimary);
    context->setDrawPrimary(m_drawPrimary);

    return rebuildRequired;
}

void RootNode::destroy() {
    // Delete all command buffers outstanding first, which means emptying the Scinth map and list.
    {
        std::lock_guard<std::mutex> lock(m_treeMutex);
        m_nodes.clear();
        m_root = nullptr;
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

bool RootNode::defAdd(std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef) {
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

void RootNode::defFree(const std::vector<std::string>& names) {
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

void RootNode::nodeFree(const std::vector<int>& nodeIDs) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (auto id : nodeIDs) {
        auto it = m_nodes.find(id);
        if (it != m_nodes.end()) {
            it->second->parent()->remove(it->second->nodeID());
            removeNode(it);
        } else {
            spdlog::warn("unable to free node ID {}, node not found.", id);
        }
    }
    m_commandBuffersDirty = true;
}

void RootNode::nodeRun(const std::vector<std::pair<int, int>>& pairs) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (const auto& pair : pairs) {
        auto it = m_nodes.find(pair.first);
        if (it != m_nodes.end()) {
            it->second->setRun(pair.second != 0);
        } else {
            spdlog::warn("unable to set run on node ID {}, node not found.", pair.first);
        }
    }
    m_commandBuffersDirty = true;
}

void RootNode::nodeSet(int nodeID, const std::vector<std::pair<std::string, float>>& namedValues,
                       const std::vector<std::pair<int, float>>& indexedValues) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    auto it = m_nodes.find(nodeID);
    if (it != m_nodes.end()) {
        it->second->setParameters(namedValues, indexedValues);
    } else {
        spdlog::warn("unable to set parameters on node ID {}, node not found.", nodeID);
    }
    m_commandBuffersDirty = true;
}

void RootNode::nodeBefore(const std::vector<std::pair<int, int>>& nodes) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (auto pair : nodes) {
        auto itA = m_nodes.find(pair.first);
        if (itA != m_nodes.end()) {
            auto itB = m_nodes.find(pair.second);
            if (itB != m_nodes.end()) {
                itA->second->parent()->remove(pair.first);
                itB->second->parent()->insertBefore(itA->second, pair.second);
            } else {
                spdlog::warn("unable to insert node {} before node {}, node {} not found", pair.first, pair.second,
                             pair.second);
            }
        } else {
            spdlog::warn("unable to insert node {} before node {}, node {} not found", pair.first, pair.second,
                         pair.first);
        }
    }
    m_commandBuffersDirty = true;
}

void RootNode::nodeAfter(const std::vector<std::pair<int, int>>& nodes) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (auto pair : nodes) {
        auto itA = m_nodes.find(pair.first);
        if (itA != m_nodes.end()) {
            auto itB = m_nodes.find(pair.second);
            if (itB != m_nodes.end()) {
                itA->second->parent()->remove(pair.first);
                itB->second->parent()->insertAfter(itA->second, pair.second);
            } else {
                spdlog::warn("unable to insert node {} after node {}, node {} not found", pair.first, pair.second,
                             pair.second);
            }
        } else {
            spdlog::warn("unable to insert node {} after node {}, node {} not found", pair.first, pair.second,
                         pair.first);
        }
    }
    m_commandBuffersDirty = true;
}

void RootNode::nodeOrder(AddAction addAction, int targetID, const std::vector<int>& nodeIDs) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    auto targetIt = m_nodes.find(targetID);
    if (targetIt != m_nodes.end()) {
        std::list<std::shared_ptr<Node>> movingNodes;
        // Build a list of pointers to the target nodes, silently ignoring any nonexistent nodes.
        for (auto id : nodeIDs) {
            if (id == targetID) {
                spdlog::warn("nodeOrder ignoring order to move node ID {} as it is also the target ID", targetID);
                continue;
            }
            auto it = m_nodes.find(id);
            if (it != m_nodes.end()) {
                // Pushing front reverses order of nodes in the list, so when traversed front-to-back it will reverse
                // it again, allowing for us to preserve original order.
                movingNodes.push_front(it->second);
            }
        }

        switch (addAction) {
        case kGroupHead: {
            if (targetIt->second->isGroup()) {
                Group* targetGroup = static_cast<Group*>(targetIt->second.get());
                for (auto node : movingNodes) {
                    node->parent()->remove(node->nodeID());
                    targetGroup->prepend(node);
                }
            } else {
                spdlog::warn("node order addToHead target ID {} not a group, ignoring", targetID);
            }
        } break;

        case kGroupTail: {
            if (targetIt->second->isGroup()) {
                Group* targetGroup = static_cast<Group*>(targetIt->second.get());
                for (auto node : movingNodes) {
                    node->parent()->remove(node->nodeID());
                    targetGroup->append(node);
                }
            } else {
                spdlog::warn("node order addToTail target ID {} not a group, ignoring", targetID);
            }
        } break;
        case kBeforeNode: {
            for (auto node : movingNodes) {
                node->parent()->remove(node->nodeID());
                targetIt->second->parent()->insertBefore(node, targetID);
            }
        } break;
        case kAfterNode: {
            for (auto node : movingNodes) {
                node->parent()->remove(node->nodeID());
                targetIt->second->parent()->insertAfter(node, targetID);
            }
        } break;

        case kReplace:
        case kActionCount:
            spdlog::warn("nodeOrder got invalid add action code {}, ignoring command.", static_cast<int>(addAction));
        }
    } else {
        spdlog::warn("unable to order nodes as target node {} not found", targetID);
    }
    m_commandBuffersDirty = true;
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
    auto scinth = std::make_shared<Scinth>(m_device, nodeID, scinthDef, m_imageMap);

    if (!scinth->create()) {
        spdlog::error("failed to build Scinth {} from ScinthDef {}.", nodeID, scinthDefName);
        return;
    }

    scinth->setParameters(namedValues, indexedValues);

    {
        std::lock_guard<std::mutex> lock(m_treeMutex);
        insertNode(scinth, addAction, targetID);
    }

    spdlog::info("Scinth id {} from def {} cueued.", nodeID, scinthDefName);

    // Will need to rebuild command buffer on next frame to include the new scinths.
    m_commandBuffersDirty = true;
}


void RootNode::groupNew(const std::vector<std::tuple<int, AddAction, int>>& groups) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (auto tuple : groups) {
        int groupID = std::get<0>(tuple);
        AddAction addAction = std::get<1>(tuple);
        int targetID = std::get<2>(tuple);

        auto group = std::make_shared<Group>(m_device, groupID);
        insertNode(group, addAction, targetID);
    }
    m_commandBuffersDirty = true;
}

void RootNode::groupHead(const std::vector<std::pair<int, int>>& nodes) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (auto pair : nodes) {
        auto groupIt = m_nodes.find(pair.first);
        if (groupIt != m_nodes.end() && groupIt->second->isGroup()) {
            Group* group = static_cast<Group*>(groupIt->second.get());
            auto nodeIt = m_nodes.find(pair.second);
            if (nodeIt != m_nodes.end()) {
                nodeIt->second->parent()->remove(pair.second);
                group->prepend(nodeIt->second);
            } else {
                spdlog::warn("groupHead node {} not found", pair.second);
            }
        } else {
            spdlog::warn("groupHead group ID {} not found or not a group", pair.first);
        }
    }
    m_commandBuffersDirty = true;
}

void RootNode::groupTail(const std::vector<std::pair<int, int>>& nodes) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (auto pair : nodes) {
        auto groupIt = m_nodes.find(pair.first);
        if (groupIt != m_nodes.end() && groupIt->second->isGroup()) {
            Group* group = static_cast<Group*>(groupIt->second.get());
            auto nodeIt = m_nodes.find(pair.second);
            if (nodeIt != m_nodes.end()) {
                nodeIt->second->parent()->remove(pair.second);
                group->append(nodeIt->second);
            } else {
                spdlog::warn("groupHead node {} not found", pair.second);
            }
        } else {
            spdlog::warn("groupHead group ID {} not found or not a group", pair.first);
        }
    }
    m_commandBuffersDirty = true;
}

void RootNode::groupFreeAll(const std::vector<int>& groupIDs) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (auto id : groupIDs) {
        auto groupIt = m_nodes.find(id);
        if (groupIt != m_nodes.end() && groupIt->second->isGroup()) {
            Group* group = static_cast<Group*>(groupIt->second.get());
            group->forEach([this](std::shared_ptr<Node> n) {
                m_nodes.erase(n->nodeID());
                if (n->isScinth()) {
                    --m_scinthCount;
                } else {
                    --m_groupCount;
                }
            });
            group->freeAll();
        } else {
            spdlog::warn("groupFreeAll id {} not found or not a group", id);
        }
    }
    m_commandBuffersDirty = true;
}

void RootNode::groupDeepFree(const std::vector<int>& groupIDs) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    for (auto id : groupIDs) {
        auto groupIt = m_nodes.find(id);
        if (groupIt != m_nodes.end() && groupIt->second->isGroup()) {
            Group* group = static_cast<Group*>(groupIt->second.get());
            group->forEach([this](std::shared_ptr<Node> n) {
                if (n->isScinth()) {
                    m_nodes.erase(n->nodeID());
                    --m_scinthCount;
                }
            });
            group->deepFree();
        } else {
            spdlog::warn("groupDeepFree id {} not found or not a group", id);
        }
    }
    m_commandBuffersDirty = true;
}

void RootNode::groupQueryTree(int groupID, std::vector<Node::NodeState>& nodes) {
    std::lock_guard<std::mutex> lock(m_treeMutex);
    auto groupIt = m_nodes.find(groupID);
    if (groupIt != m_nodes.end() && groupIt->second->isGroup()) {
        Group* group = static_cast<Group*>(groupIt->second.get());
        group->queryTree(nodes);
    } else {
        spdlog::warn("groupQueryTree id {} not found or not a group", groupID);
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
        std::lock_guard<std::mutex> lock(m_treeMutex);
        m_audioStagers.push_back(stager);
    }
    return true;
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

    m_commandBuffersDirty = false;
}

void RootNode::insertNode(std::shared_ptr<Node> node, AddAction addAction, int targetID) {
    auto existingNode = m_nodes.find(node->nodeID());
    // Remove existing node of same ID, unless the command is to replace existing node of same ID, as it will be
    // removed as part of the replace operation.
    if (existingNode != m_nodes.end()
        && ((addAction != kReplace) || (addAction == kReplace && node->nodeID() != targetID))) {
        spdlog::warn("clobbering existing nodeID {}");
        existingNode->second->parent()->remove(node->nodeID());
        removeNode(existingNode);
    }
    auto targetNode = m_nodes.find(targetID);
    if (targetNode != m_nodes.end()) {
        switch (addAction) {
        case kGroupHead: {
            if (targetNode->second->isGroup()) {
                Group* targetGroup = static_cast<Group*>(targetNode->second.get());
                targetGroup->prepend(node);
                m_nodes[node->nodeID()] = node;
            } else {
                spdlog::error("add node {} to group head target {} not a group, ignoring", node->nodeID(), targetID);
                return;
            }
        } break;
        case kGroupTail: {
            if (targetNode->second->isGroup()) {
                Group* targetGroup = static_cast<Group*>(targetNode->second.get());
                targetGroup->append(node);
                m_nodes[node->nodeID()] = node;
            } else {
                spdlog::error("add node {} to group tail target {} not a group, ignoring", node->nodeID(), targetID);
                return;
            }
        } break;
        case kBeforeNode: {
            targetNode->second->parent()->insertBefore(node, targetID);
            m_nodes[node->nodeID()] = node;
        } break;
        case kAfterNode: {
            targetNode->second->parent()->insertAfter(node, targetID);
            m_nodes[node->nodeID()] = node;
        } break;
        case kReplace: {
            targetNode->second->parent()->replace(node, targetID);
            removeNode(targetNode);
            m_nodes[node->nodeID()] = node;
        } break;
        case kActionCount:
            spdlog::error("new got unknown addAction value, ignoring.");
            return;
        }

        if (node->isScinth()) {
            ++m_scinthCount;
        } else {
            ++m_groupCount;
        }
    } else {
        spdlog::error("new couldn't find target node ID {}, ignoring", targetID);
    }
}

void RootNode::removeNode(std::unordered_map<int, std::shared_ptr<Node>>::iterator it) {
    it->second->forEach([this](std::shared_ptr<Node> n) {
        m_nodes.erase(n->nodeID());
        if (n->isScinth()) {
            --m_scinthCount;
        } else {
            --m_groupCount;
        }
    });
    if (it->second->isScinth()) {
        --m_scinthCount;
    } else {
        --m_groupCount;
    }
    m_nodes.erase(it);
}

} // namespace comp
} // namespace scin
