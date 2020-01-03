#include "Scinth.hpp"

#include "core/AbstractScinthDef.hpp"
#include "core/Shape.hpp"
#include "core/VGen.hpp"
#include "vulkan/Canvas.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/Pipeline.hpp"
#include "vulkan/Uniform.hpp"

#include "spdlog/spdlog.h"

namespace scin {

Scinth::Scinth(std::shared_ptr<vk::Device> device, const std::string& name,
               std::shared_ptr<const AbstractScinthDef> abstractScinthDef):
    m_device(device),
    m_name(name),
    m_abstractScinthDef(abstractScinthDef) {}

Scinth::~Scinth() {}

bool Scinth::create(const TimePoint& startTime, vk::UniformLayout* uniformLayout, size_t numberOfImages) {
    m_startTime = startTime;
    if (uniformLayout) {
        m_uniform.reset(new vk::Uniform(m_device));
        if (!m_uniform->createBuffers(uniformLayout, m_abstractScinthDef->uniformManifest().sizeInBytes(),
                                      numberOfImages)) {
            spdlog::error("failed creating uniform buffers for Scinth {}", m_name);
            return false;
        }
    }

    return true;
}

bool Scinth::buildBuffers(vk::CommandPool* commandPool, vk::Canvas* canvas, vk::Buffer* vertexBuffer,
                          vk::Buffer* indexBuffer, vk::Pipeline* pipeline) {
    m_commands = commandPool->createBuffers(canvas->numberOfImages(), false);
    if (!m_commands) {
        spdlog::error("failed creating command buffers for Scinth {}", m_name);
        return false;
    }

    for (auto i = 0; i < canvas->numberOfImages(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags =
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = canvas->renderPass();
        inheritanceInfo.subpass = 0; // TODO: alpha-blended Scinths might need their own subpasses?
        inheritanceInfo.framebuffer = canvas->framebuffer(i);
        beginInfo.pInheritanceInfo = &inheritanceInfo;

        if (vkBeginCommandBuffer(m_commands->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::error("failed beginning command buffer {} for Scinth {}", i, m_name);
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

        vkCmdDrawIndexed(m_commands->buffer(i), m_abstractScinthDef->shape()->numberOfIndices(), 1, 0, 0, 0);

        if (vkEndCommandBuffer(m_commands->buffer(i)) != VK_SUCCESS) {
            spdlog::error("failed ending command buffer {} for Scinth {}", i, m_name);
            return false;
        }
    }

    return true;
}

bool Scinth::prepareFrame(size_t imageIndex, const TimePoint& frameTime) {
    // Update the Uniform buffer at imageIndex, if needed.
    if (m_uniform) {
        std::unique_ptr<float[]> uniformData(
            new float[m_abstractScinthDef->uniformManifest().sizeInBytes() / sizeof(float)]);
        float* uniform = uniformData.get();
        for (auto i = 0; i < m_abstractScinthDef->uniformManifest().numberOfElements(); ++i) {
            switch (m_abstractScinthDef->uniformManifest().intrinsicForElement(i)) {
            case kNormPos:
                spdlog::error("normPos is not a valid intrinsic for a Uniform in Scinth {}", m_name);
                return false;

            case kTime:
                *uniform = std::chrono::duration<float, std::chrono::seconds::period>(frameTime - m_startTime).count();
                break;

            case kNotFound:
                spdlog::error("unknown uniform Intrinsic in Scinth {}", m_name);
                return false;
            }

            uniform += (m_abstractScinthDef->uniformManifest().strideForElement(i) / sizeof(float));
        }

        m_uniform->buffer(imageIndex)->copyToGPU(uniformData.get());
    }

    return true;
}

} // namespace scin
