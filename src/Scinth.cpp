#include "Scinth.hpp"

#include "ScinthDef.hpp"
#include "core/AbstractScinthDef.hpp"
#include "core/Shape.hpp"
#include "core/VGen.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/Canvas.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Pipeline.hpp"
#include "vulkan/Uniform.hpp"

#include "spdlog/spdlog.h"

namespace scin {

Scinth::Scinth(std::shared_ptr<vk::Device> device, int nodeID, std::shared_ptr<ScinthDef> scinthDef):
    m_device(device),
    m_nodeID(nodeID),
    m_scinthDef(scinthDef),
    m_running(false) {}

Scinth::~Scinth() { spdlog::debug("Scinth destructor"); }

bool Scinth::create(const TimePoint& startTime, vk::UniformLayout* uniformLayout, size_t numberOfImages) {
    m_startTime = startTime;
    m_running = true;
    if (uniformLayout) {
        m_uniform.reset(new vk::Uniform(m_device));
        if (!m_uniform->createBuffers(uniformLayout, m_scinthDef->abstract()->uniformManifest().sizeInBytes(),
                                      numberOfImages)) {
            spdlog::error("failed creating uniform buffers for Scinth {}", m_nodeID);
            return false;
        }
    }

    return true;
}

bool Scinth::buildBuffers(vk::CommandPool* commandPool, vk::Canvas* canvas, std::shared_ptr<vk::Buffer> vertexBuffer,
                          std::shared_ptr<vk::Buffer> indexBuffer, std::shared_ptr<vk::Pipeline> pipeline) {
    m_commands = commandPool->createBuffers(canvas->numberOfImages(), false);
    if (!m_commands) {
        spdlog::error("failed creating command buffers for Scinth {}", m_nodeID);
        return false;
    }

    m_commands->associateResources(vertexBuffer, indexBuffer, m_uniform, pipeline);

    for (auto i = 0; i < canvas->numberOfImages(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags =
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = canvas->renderPass();
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = canvas->framebuffer(i);
        beginInfo.pInheritanceInfo = &inheritanceInfo;

        if (vkBeginCommandBuffer(m_commands->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::error("failed beginning command buffer {} for Scinth {}", i, m_nodeID);
            return false;
        }

        vkCmdBindPipeline(m_commands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get());
        VkBuffer vertexBuffers[] = { vertexBuffer->buffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commands->buffer(i), 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_commands->buffer(i), indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT16);

        if (m_uniform) {
            vkCmdBindDescriptorSets(m_commands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout(), 0, 1,
                                    m_uniform->set(i), 0, nullptr);
        }

        vkCmdDrawIndexed(m_commands->buffer(i), m_scinthDef->abstract()->shape()->numberOfIndices(), 1, 0, 0, 0);

        if (vkEndCommandBuffer(m_commands->buffer(i)) != VK_SUCCESS) {
            spdlog::error("failed ending command buffer {} for Scinth {}", i, m_nodeID);
            return false;
        }
    }

    return true;
}

bool Scinth::prepareFrame(size_t imageIndex, const TimePoint& frameTime) {
    // Update the Uniform buffer at imageIndex, if needed.
    if (m_uniform) {
        std::unique_ptr<float[]> uniformData(
            new float[m_scinthDef->abstract()->uniformManifest().sizeInBytes() / sizeof(float)]);
        float* uniform = uniformData.get();
        for (auto i = 0; i < m_scinthDef->abstract()->uniformManifest().numberOfElements(); ++i) {
            switch (m_scinthDef->abstract()->uniformManifest().intrinsicForElement(i)) {
            case core::Intrinsic::kNormPos:
                spdlog::error("normPos is not a valid intrinsic for a Uniform in Scinth {}", m_nodeID);
                return false;

            case core::Intrinsic::kPi:
                spdlog::error("pi not a valid Uniform intrinsic in Scinth {}", m_nodeID);
                return false;

            case core::Intrinsic::kTime:
                *uniform = std::chrono::duration<float, std::chrono::seconds::period>(frameTime - m_startTime).count();
                break;

            case core::Intrinsic::kNotFound:
                spdlog::error("unknown uniform Intrinsic in Scinth {}", m_nodeID);
                return false;
            }

            uniform += (m_scinthDef->abstract()->uniformManifest().strideForElement(i) / sizeof(float));
        }

        m_uniform->buffer(imageIndex)->copyToGPU(uniformData.get());
    }

    return true;
}

} // namespace scin
