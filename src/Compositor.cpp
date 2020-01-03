#include "Compositor.hpp"

#include "Scinth.hpp"
#include "ScinthDef.hpp"
#include "core/AbstractScinthDef.hpp"
#include "core/VGen.hpp"
#include "vulkan/Canvas.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/ShaderCompiler.hpp"

#include "spdlog/spdlog.h"

namespace scin {

Compositor::Compositor(std::shared_ptr<vk::Device> device, std::shared_ptr<vk::Canvas> canvas):
    m_device(device),
    m_canvas(canvas),
    m_clearColor(1.0, 1.0, 1.0),
    m_shaderCompiler(new scin::vk::ShaderCompiler()),
    m_commandPool(new scin::vk::CommandPool(device)),
    m_commandBufferDirty(true) {
    m_frameCommands.resize(m_canvas->numberOfImages());
}

Compositor::~Compositor() {}

bool Compositor::create() {
    if (!m_shaderCompiler->loadCompiler()) {
        spdlog::error("unable to load shader compiler.");
        return false;
    }

    if (!m_commandPool->create()) {
        spdlog::error("error creating command pool.");
        return false;
    }

    rebuildCommandBuffer();
    return true;
}

bool Compositor::buildScinthDef(std::shared_ptr<const AbstractScinthDef> abstractScinthDef) {
    std::shared_ptr<ScinthDef> scinthDef(new ScinthDef(m_device, m_canvas, abstractScinthDef));
    if (!scinthDef->build(m_shaderCompiler.get())) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_scinthDefMutex);
        m_scinthDefs.insert_or_assign(abstractScinthDef->name(), std::move(scinthDef));
    }

    return true;
}

bool Compositor::play(const std::string& scinthDefName, const std::string& scinthName, const TimePoint& startTime) {
    std::shared_ptr<ScinthDef> scinthDef;
    {
        std::lock_guard<std::mutex> lock(m_scinthDefMutex);
        auto it = m_scinthDefs.find(scinthDefName);
        if (it != m_scinthDefs.end()) {
            scinthDef = it->second;
        }
    }
    if (!scinthDef) {
        spdlog::error("ScinthDef {} not found when building Scinth {}.", scinthDefName, scinthName);
        return false;
    }
    std::shared_ptr<Scinth> scinth = scinthDef->play(scinthName, startTime);
    if (!scinth) {
        spdlog::error("failed to build Scinth {} from ScinthDef {}.", scinthName, scinthDefName);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        auto map = m_scinthMap.find(scinthName);
        if (map != m_scinthMap.end()) {
            spdlog::info("clobbering existing Scinth named {}", scinthName);
            stopScinthLockAcquired(map);
        }
        m_scinths.push_back(scinth);
        auto it = m_scinths.end();
        --it;
        m_scinthMap.insert({ scinthName, it });
    }

    // Will need to rebuild command buffer on next frame to include the new scinths.
    m_commandBufferDirty = true;
    return true;
}

std::shared_ptr<vk::CommandBuffer> Compositor::prepareFrame(uint32_t imageIndex, const TimePoint& frameTime) {
    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        for (auto scinth : m_scinths) {
            scinth->prepareFrame(imageIndex, frameTime);
        }
    }
    if (m_commandBufferDirty) {
        rebuildCommandBuffer();
    }
    return m_primaryCommands;
}

void Compositor::releaseCompiler() { m_shaderCompiler->releaseCompiler(); }

void Compositor::destroy() { m_commandPool->destroy(); }

// Needs to be called only from the same thread that calls prepareFrame. Assumes that m_secondaryCommands is up-to-date.
bool Compositor::rebuildCommandBuffer() {
    m_primaryCommands = m_commandPool->createBuffers(m_canvas->numberOfImages(), true);
    if (!m_primaryCommands) {
        spdlog::critical("failed creating primary command buffers for Compositor.");
        return false;
    }

    VkClearColorValue clearColor = { m_clearColor.x, m_clearColor.y, m_clearColor.z, 1.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    for (auto i = 0; i < m_canvas->numberOfImages(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(m_primaryCommands->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::error("failed beginning clear command buffer for Compositor");
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
            return false;
        }
    }

    m_commandBufferDirty = false;
    return true;
}

void Compositor::stopScinthLockAcquired(ScinthMap::iterator it) {
    // Remove from list first, then dictionary,
    m_scinths.erase(it->second);
    m_scinthMap.erase(it);
}

} // namespace scin
