#include "comp/RootNode.hpp"

#include "comp/Canvas.hpp"
#include "vulkan/Device.hpp"

#include <spdlog/spdlog.h>

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
    m_nodeSerial(-2) {
    m_computeCommands.resize(m_canvas->numberOfImages());
    m_drawCommands.resize(m_canvas->numberOfImages());
}

bool RootNode::create() {
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
                return false;
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

void RootNode::destroy() {
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

bool RootNode::prepareFrame(std::shared_ptr<FrameContext> context) {
    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
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

    context->setComputePrimaryCommands(m_computePrimary);
    context->setDrawPrimaryCommands(m_drawPrimary);

    return rebuildRequired;
}

void RootNode::rebuildCommandBuffer(std::shared_ptr<FrameContext> context) {
    if (context->computeCommands().size()) {
        m_computePrimary.reset(new vk::CommandBuffer(m_device, m_computeCommandPool));
        if (!m_computePrimary->create(m_canvas->numberOfImages(), true)) {
            spdlog::critical("failed creating primary compute command buffers for Compositor.");
            return false;
        }

        spdlog::debug("rebuilding Compositor compute command buffer with {} secondary command buffers",
                      m_computeSecondary.size());

        for (size_t i = 0; i < m_canvas->numberOfImages(); ++i) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(m_computePrimary->buffer(i), &beginInfo) != VK_SUCCESS) {
                spdlog::error("Compositor failed beginning primary compute command buffer.");
                return false;
            }

            std::vector<VkCommandBuffer> commandBuffers;
            for (const auto command : context->computeCommands()) {
                commandBuffers.emplace_back(command->buffer(i));
            }
            vkCmdExecuteCommands(m_computePrimary->buffer(i), static_cast<uint32_t>(commandBuffers.size()),
                                 commandBuffers.data());

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

    /*
    VkClearColorValue clearColor = { { m_clearColor.x, m_clearColor.y, m_clearColor.z, 1.0f } };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;
    */

    spdlog::debug("rebuilding Compositor draw command buffer with {} secondary command buffers",
                  m_drawSecondary.size());

    for (size_t i = 0; i < m_canvas->numberOfImages(); ++i) {
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
            spdlog::error("Compositor failed ending primary draw command buffer.");
            return false;
        }
    }

    m_commandBufferDirty = false;
    return true;
}


} // namespace comp
} // namespace scin
