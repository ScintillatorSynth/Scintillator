#include "comp/Scinth.hpp"

#include "base/AbstractScinthDef.hpp"
#include "base/Shape.hpp"
#include "base/VGen.hpp"
#include "comp/Canvas.hpp"
#include "comp/ImageMap.hpp"
#include "comp/Pipeline.hpp"
#include "comp/SamplerFactory.hpp"
#include "comp/ScinthDef.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/CommandBuffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"
#include "vulkan/Sampler.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

Scinth::Scinth(std::shared_ptr<vk::Device> device, int nodeID, std::shared_ptr<ScinthDef> scinthDef,
               std::shared_ptr<ImageMap> imageMap):
    m_device(device),
    m_nodeID(nodeID),
    m_cueued(true),
    m_scinthDef(scinthDef),
    m_imageMap(imageMap),
    m_descriptorPool(VK_NULL_HANDLE),
    m_running(false),
    m_numberOfParameters(0),
    m_commandBuffersDirty(true) {}

Scinth::~Scinth() {
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device->get(), m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
}

bool Scinth::create() {
    m_running = true;

    if (!allocateDescriptors()) {
        spdlog::error("Scinth {} failed to build descriptors.", m_nodeID);
        return false;
    }

    m_numberOfParameters = m_scinthDef->abstract()->parameters().size();
    if (m_numberOfParameters) {
        m_parameterValues.reset(new float[m_numberOfParameters]);
        for (auto i = 0; i < m_numberOfParameters; ++i) {
            m_parameterValues[i] = m_scinthDef->abstract()->parameters()[i].defaultValue();
        }
    }

    updateDescriptors();
    return rebuildBuffers();
}

bool Scinth::prepareFrame(size_t imageIndex, double frameTime) {
    // If this is our first call to prepareFrame we treat this frameTime as our startTime.
    if (m_cueued) {
        m_startTime = frameTime;
        m_cueued = false;
    }

    // Update the Uniform buffer at imageIndex, if needed.
    if (m_uniformBuffers.size()) {
        float* uniform = static_cast<float*>(m_uniformBuffers[imageIndex]->mappedAddress());
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

bool Scinth::allocateDescriptors() {
    // No layout means no image or uniform arguments, nothing to do, return.
    if (m_scinthDef->layout() == VK_NULL_HANDLE) {
        return true;
    }

    auto numberOfImages = m_scinthDef->canvas()->numberOfImages();
    std::vector<VkDescriptorPoolSize> poolSizes;

    auto uniformSize = m_scinthDef->abstract()->uniformManifest().sizeInBytes();
    if (uniformSize) {
        VkDescriptorPoolSize uniformPoolSize = {};
        uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformPoolSize.descriptorCount = numberOfImages;
        poolSizes.emplace_back(uniformPoolSize);
    }

    auto totalSamplers =
        m_scinthDef->abstract()->fixedImages().size() + m_scinthDef->abstract()->parameterizedImages().size();
    if (totalSamplers) {
        VkDescriptorPoolSize samplerPoolSize = {};
        samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerPoolSize.descriptorCount = numberOfImages * totalSamplers;
        poolSizes.emplace_back(samplerPoolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = numberOfImages;
    if (vkCreateDescriptorPool(m_device->get(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        spdlog::error("Scinth {} failed to create Vulkan descriptor pool.", m_nodeID);
        return false;
    }

    std::vector<VkDescriptorSetLayout> layouts(numberOfImages, m_scinthDef->layout());
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = numberOfImages;
    allocInfo.pSetLayouts = layouts.data();
    m_descriptorSets.resize(numberOfImages);
    if (vkAllocateDescriptorSets(m_device->get(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        spdlog::error("Scinth {} failed to allocate Vulkan descriptor sets.", m_nodeID);
        return false;
    }

    // Write the constant parts of the descriptor sets only once, those are the uniforms and fixed images.
    for (auto i = 0; i < numberOfImages; ++i) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        VkDescriptorBufferInfo bufferInfo = {};
        int32_t binding = 0;
        if (uniformSize) {
            bufferInfo.buffer = m_uniformBuffers[i]->buffer();
            bufferInfo.offset = 0;
            bufferInfo.range = m_uniformBuffers[i]->size();

            VkWriteDescriptorSet bufferWrite = {};
            bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            bufferWrite.dstSet = m_descriptorSets[i];
            bufferWrite.dstBinding = binding;
            bufferWrite.dstArrayElement = 0;
            bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bufferWrite.descriptorCount = 1;
            bufferWrite.pBufferInfo = &bufferInfo;
            bufferWrite.pImageInfo = nullptr;
            bufferWrite.pTexelBufferView = nullptr;
            descriptorWrites.emplace_back(bufferWrite);

            ++binding;
        }

        std::vector<VkDescriptorImageInfo> imageInfos;
        auto samplerIndex = 0;
        for (const auto pair : m_scinthDef->abstract()->fixedImages()) {
            std::shared_ptr<vk::DeviceImage> image = m_imageMap->getImage(pair.second);
            if (!image) {
                spdlog::error("Scinth {} failed to find image ID {}.", m_nodeID, pair.second);
                return false;
            }

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = image->view();
            imageInfo.sampler = m_scinthDef->fixedSamplers()[samplerIndex]->get();
            imageInfos.emplace_back(imageInfo);

            ++samplerIndex;
            m_fixedImages.emplace_back(image);

            VkWriteDescriptorSet imageWrite = {};
            imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWrite.dstSet = m_descriptorSets[i];
            imageWrite.dstBinding = binding;
            imageWrite.dstArrayElement = 0;
            imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            imageWrite.descriptorCount = 1;
            imageWrite.pImageInfo = imageInfos.data() + (imageInfos.size() - 1);
            descriptorWrites.emplace_back(imageWrite);

            ++binding;
        }

        vkUpdateDescriptorSets(m_device->get(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

    return true;
}

void Scinth::updateDescriptors() {
    for (const auto pair : m_scinthDef->abstract()->parameterizedImages()) {
    }
}

bool Scinth::rebuildBuffers() {
    m_commands.reset(new vk::CommandBuffer(m_device, m_scinthDef->commandPool()));
    if (!m_commands->create(m_scinthDef->canvas()->numberOfImages(), false)) {
        spdlog::error("failed creating command buffers for Scinth {}", m_nodeID);
        return false;
    }

    m_commands->associateResources(m_scinthDef->vertexBuffer(), m_scinthDef->indexBuffer(), m_scinthDef->pipeline());

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

        if (m_descriptorSets.size()) {
            vkCmdBindDescriptorSets(m_commands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_scinthDef->pipeline()->layout(), 0, 1, &m_descriptorSets[i], 0, nullptr);
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

} // namespace comp

} // namespace scin
