#include "Scinth.hpp"

#include "ScinthDef.hpp"
#include "base/AbstractScinthDef.hpp"
#include "base/Shape.hpp"
#include "base/VGen.hpp"
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
    m_cueued(true),
    m_scinthDef(scinthDef),
    m_running(false),
    m_numberOfParameters(0),
    m_commandBuffersDirty(true) {}

Scinth::~Scinth() { spdlog::debug("Scinth {} destructor", m_nodeID); }

bool Scinth::create() {
    m_running = true;
    if (m_scinthDef->uniformLayout()) {
        m_uniform.reset(new vk::Uniform(m_device));
        if (!m_uniform->createBuffers(m_scinthDef->uniformLayout().get(),
                                      m_scinthDef->abstract()->uniformManifest().sizeInBytes(),
                                      m_scinthDef->canvas()->numberOfImages())) {
            spdlog::error("failed creating uniform buffers for Scinth {}", m_nodeID);
            return false;
        }
    }

    m_numberOfParameters = m_scinthDef->abstract()->parameters().size();
    if (m_numberOfParameters) {
        m_parameterValues.reset(new float[m_numberOfParameters]);
        for (auto i = 0; i < m_numberOfParameters; ++i) {
            m_parameterValues[i] = m_scinthDef->abstract()->parameters()[i].defaultValue();
        }
    }

    return rebuildBuffers();
}


bool Scinth::prepareFrame(size_t imageIndex, double frameTime) {
    // If this is our first call to prepareFrame we treat this frameTime as our startTime.
    if (m_cueued) {
        m_startTime = frameTime;
        m_cueued = false;
    }

    // Update the Uniform buffer at imageIndex, if needed.
    if (m_uniform) {
        std::unique_ptr<float[]> uniformData(
            new float[m_scinthDef->abstract()->uniformManifest().sizeInBytes() / sizeof(float)]);
        float* uniform = uniformData.get();
        for (auto i = 0; i < m_scinthDef->abstract()->uniformManifest().numberOfElements(); ++i) {
            switch (m_scinthDef->abstract()->uniformManifest().intrinsicForElement(i)) {
            case base::Intrinsic::kTime:
                *uniform = static_cast<float>(frameTime - m_startTime);
                break;

            case base::Intrinsic::kNormPos:
            case base::Intrinsic::kNotFound:
            case base::Intrinsic::kPi:
            case base::Intrinsic::kSampler:
            case base::Intrinsic::kTexPos:
                spdlog::error("Unknown or invalid uniform Intrinsic in Scinth {}", m_nodeID);
                return false;
            }

            uniform += (m_scinthDef->abstract()->uniformManifest().strideForElement(i) / sizeof(float));
        }

        m_uniform->buffer(imageIndex)->copyToGPU(uniformData.get());
    }

    if (m_commandBuffersDirty) {
        return rebuildBuffers();
    }

    return true;
}

void Scinth::setParameterByName(const std::string& name, float value) {
    int index = m_scinthDef->abstract()->indexForParameterName(name);
    if (index >= 0) {
        m_parameterValues[index] = value;
        m_commandBuffersDirty = true;
    } else {
        spdlog::warn("Scinth {} failed to find parameter named {}", m_nodeID, name);
    }
}

void Scinth::setParameterByIndex(int index, float value) {
    m_parameterValues[index] = value;
    m_commandBuffersDirty = true;
}

bool Scinth::rebuildBuffers() {
    m_commands.reset(new vk::CommandBuffer(m_device, m_scinthDef->commandPool()));
    if (!m_commands->create(m_scinthDef->canvas()->numberOfImages(), false)) {
        spdlog::error("failed creating command buffers for Scinth {}", m_nodeID);
        return false;
    }

    m_commands->associateResources(m_scinthDef->vertexBuffer(), m_scinthDef->indexBuffer(), m_uniform,
                                   m_scinthDef->pipeline());

    for (auto i = 0; i < m_scinthDef->canvas()->numberOfImages(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags =
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = m_scinthDef->canvas()->renderPass();
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = m_scinthDef->canvas()->framebuffer(i);
        beginInfo.pInheritanceInfo = &inheritanceInfo;

        if (vkBeginCommandBuffer(m_commands->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::error("failed beginning command buffer {} for Scinth {}", i, m_nodeID);
            return false;
        }

        if (m_numberOfParameters) {
            vkCmdPushConstants(m_commands->buffer(i), m_scinthDef->pipeline()->layout(), VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, m_numberOfParameters * sizeof(float), m_parameterValues.get());
        }

        vkCmdBindPipeline(m_commands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS, m_scinthDef->pipeline()->get());
        VkBuffer vertexBuffers[] = { m_scinthDef->vertexBuffer()->buffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commands->buffer(i), 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_commands->buffer(i), m_scinthDef->indexBuffer()->buffer(), 0, VK_INDEX_TYPE_UINT16);

        if (m_uniform) {
            vkCmdBindDescriptorSets(m_commands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_scinthDef->pipeline()->layout(), 0, 1, m_uniform->set(i), 0, nullptr);
        }

        vkCmdDrawIndexed(m_commands->buffer(i), m_scinthDef->abstract()->shape()->numberOfIndices(), 1, 0, 0, 0);

        if (vkEndCommandBuffer(m_commands->buffer(i)) != VK_SUCCESS) {
            spdlog::error("failed ending command buffer {} for Scinth {}", i, m_nodeID);
            return false;
        }
    }

    m_commandBuffersDirty = false;
    return true;
}

} // namespace scin
