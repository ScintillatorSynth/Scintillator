#include "Scinth.hpp"

#include "core/Shape.hpp"
#include "vulkan/Canvas.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/Pipeline.hpp"
#include "vulkan/Uniform.hpp"

#include "spdlog/spdlog.h"

namespace scin {

Scinth::Scinth(const std::string& name): m_name(name) {}

Scinth::~Scinth() {}

// could be called multiple times? like with parameter change?
bool Scinth::build(vk::CommandPool* commandPool, vk::Canvas* canvas, vk::Buffer* vertexBuffer, vk::Buffer* indexBuffer,
                   vk::Pipeline* pipeline, vk::Uniform* uniform, Shape* shape) {
    m_commands = commandPool->createBuffers(canvas->numberOfImages());
    if (!m_commands) {
        spdlog::error("failed creating command buffers for Scinth {}", m_name);
        return false;
    }

    for (size_t i = 0; i < canvas->numberOfImages(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(m_commands->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::error("failed beginning command buffer {} for Scinth {}", i, m_name);
            return false;
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = canvas->renderPass();
        renderPassInfo.framebuffer = canvas->framebuffer(i);
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = canvas->extent();

        vkCmdBeginRenderPass(m_commands->buffer(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_commands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get());
        VkBuffer vertexBuffers[] = { vertexBuffer->buffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commands->buffer(i), 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_commands->buffer(i), indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT16);

        if (uniform) {
            vkCmdBindDescriptorSets(m_commands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout(), 0, 1,
                                    uniform->set(i), 0, nullptr);
        }

        vkCmdDrawIndexed(m_commands->buffer(i), shape->numberOfIndices(), 1, 0, 0, 0);
        vkCmdEndRenderPass(m_commands->buffer(i));
        if (vkEndCommandBuffer(m_commands->buffer(i)) != VK_SUCCESS) {
            spdlog::error("failed ending command buffer {} for Scinth {}", i, m_name);
            return false;
        }
    }

    return true;
}

} // namespace scin
