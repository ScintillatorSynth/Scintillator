#include "comp/Scinth.hpp"

#include "base/AbstractScinthDef.hpp"
#include "base/Shape.hpp"
#include "base/VGen.hpp"
#include "comp/Canvas.hpp"
#include "comp/FrameContext.hpp"
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
    Node(device, nodeID),
    m_cueued(true),
    m_scinthDef(scinthDef),
    m_imageMap(imageMap),
    m_descriptorPool(VK_NULL_HANDLE),
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
        for (size_t i = 0; i < m_numberOfParameters; ++i) {
            m_parameterValues[i] = m_scinthDef->abstract()->parameters()[i].defaultValue();
        }
    }

    return true;
}

bool Scinth::prepareFrame(std::shared_ptr<FrameContext> context) {
    // If this is our first call to prepareFrame we treat this frameTime as our startTime.
    if (m_cueued) {
        m_startTime = context->frameTime();
        m_cueued = false;
    }

    // Update the Uniform buffer at imageIndex, if needed.
    if (m_uniformBuffers.size()) {
        float* uniform = static_cast<float*>(m_uniformBuffers[context->imageIndex()]->mappedAddress());
        for (size_t i = 0; i < m_scinthDef->abstract()->uniformManifest().numberOfElements(); ++i) {
            switch (m_scinthDef->abstract()->uniformManifest().intrinsicForElement(i)) {
            case base::Intrinsic::kTime:
                *uniform = static_cast<float>(context->frameTime() - m_startTime);
                break;

            case base::Intrinsic::kFragCoord:
            case base::Intrinsic::kNormPos:
            case base::Intrinsic::kNotFound:
            case base::Intrinsic::kPi:
            case base::Intrinsic::kPosition:
            case base::Intrinsic::kSampler:
            case base::Intrinsic::kTexPos:
                spdlog::critical("Unknown or invalid uniform Intrinsic in Scinth {}", m_nodeID);
                return true;
            }

            uniform += (m_scinthDef->abstract()->uniformManifest().strideForElement(i) / sizeof(float));
        }
    }

    bool rebuildRequired = m_commandBuffersDirty;
    if (m_commandBuffersDirty) {
        updateDescriptors();
        rebuildBuffers();
    }

    if (m_computeCommands) {
        context->appendComputeCommands(m_computeCommands);
    }
    context->appendDrawCommands(m_drawCommands);
    for (auto image : m_fixedImages) {
        context->appendImage(image);
    }
    for (auto image : m_parameterizedImages) {
        context->appendImage(image);
    }

    return rebuildRequired;
}

void Scinth::setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                           const std::vector<std::pair<int, float>>& indexedValues) {
    for (auto namedPair : namedValues) {
        size_t index = 0;
        if (m_scinthDef->abstract()->indexForParameterName(namedPair.first, index)) {
            m_parameterValues[index] = namedPair.second;
            m_commandBuffersDirty = true;
        } else {
            spdlog::info("Scinth {} failed to find parameter named {}", m_nodeID, namedPair.first);
        }
    }

    for (auto indexPair : indexedValues) {
        m_parameterValues[indexPair.first] = indexPair.second;
    }

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
        uniformPoolSize.descriptorCount = static_cast<uint32_t>(numberOfImages);
        poolSizes.emplace_back(uniformPoolSize);

        for (size_t i = 0; i < numberOfImages; ++i) {
            std::shared_ptr<vk::HostBuffer> uniformBuffer(
                new vk::HostBuffer(m_device, vk::Buffer::Kind::kUniform, uniformSize));
            if (!uniformBuffer->create()) {
                spdlog::error("Scinth {} failed to create uniform buffer of size {}", m_nodeID, uniformSize);
                return false;
            }
            m_uniformBuffers.emplace_back(uniformBuffer);
        }
    }

    auto totalSamplers =
        m_scinthDef->abstract()->drawFixedImages().size() + m_scinthDef->abstract()->drawParameterizedImages().size();
    if (totalSamplers) {
        VkDescriptorPoolSize samplerPoolSize = {};
        samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerPoolSize.descriptorCount = static_cast<uint32_t>(numberOfImages * totalSamplers);
        poolSizes.emplace_back(samplerPoolSize);
    }

    auto computeBufferSize = m_scinthDef->abstract()->computeManifest().sizeInBytes();
    if (computeBufferSize) {
        VkDescriptorPoolSize bufferPoolSize = {};
        bufferPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bufferPoolSize.descriptorCount = static_cast<uint32_t>(numberOfImages);
        poolSizes.emplace_back(bufferPoolSize);

        for (size_t i = 0; i < numberOfImages; ++i) {
            std::shared_ptr<vk::DeviceBuffer> computeBuffer(
                new vk::DeviceBuffer(m_device, vk::Buffer::Kind::kStorage, computeBufferSize));
            if (!computeBuffer->create()) {
                spdlog::error("Scinth {} failed to create compute storage buffer of size {}", m_nodeID,
                              computeBufferSize);
                return false;
            }
            m_computeBuffers.emplace_back(computeBuffer);
        }
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(numberOfImages);
    if (vkCreateDescriptorPool(m_device->get(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        spdlog::error("Scinth {} failed to create Vulkan descriptor pool.", m_nodeID);
        return false;
    }

    std::vector<VkDescriptorSetLayout> layouts(numberOfImages, m_scinthDef->layout());
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(numberOfImages);
    allocInfo.pSetLayouts = layouts.data();
    m_descriptorSets.resize(numberOfImages);
    if (vkAllocateDescriptorSets(m_device->get(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        spdlog::error("Scinth {} failed to allocate Vulkan descriptor sets.", m_nodeID);
        return false;
    }

    for (size_t i = 0; i < numberOfImages; ++i) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        VkDescriptorBufferInfo uniformBufferInfo = {};
        int32_t binding = 0;
        if (uniformSize) {
            uniformBufferInfo.buffer = m_uniformBuffers[i]->buffer();
            uniformBufferInfo.offset = 0;
            uniformBufferInfo.range = m_uniformBuffers[i]->size();

            VkWriteDescriptorSet bufferWrite = {};
            bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            bufferWrite.dstSet = m_descriptorSets[i];
            bufferWrite.dstBinding = binding;
            bufferWrite.dstArrayElement = 0;
            bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bufferWrite.descriptorCount = 1;
            bufferWrite.pBufferInfo = &uniformBufferInfo;
            bufferWrite.pImageInfo = nullptr;
            bufferWrite.pTexelBufferView = nullptr;
            descriptorWrites.emplace_back(bufferWrite);

            ++binding;
        }

        std::vector<VkDescriptorImageInfo> imageInfos(totalSamplers);
        auto samplerIndex = 0;
        auto imageInfoIndex = 0;
        for (const auto pair : m_scinthDef->abstract()->drawFixedImages()) {
            std::shared_ptr<vk::DeviceImage> image = m_imageMap->getImage(pair.second);
            if (!image) {
                spdlog::error("Scinth {} failed to find constant image ID {}.", m_nodeID, pair.second);
                return false;
            }

            imageInfos[imageInfoIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[imageInfoIndex].imageView = image->view();
            imageInfos[imageInfoIndex].sampler = m_scinthDef->fixedSamplers()[samplerIndex]->get();

            VkWriteDescriptorSet imageWrite = {};
            imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWrite.dstSet = m_descriptorSets[i];
            imageWrite.dstBinding = binding;
            imageWrite.dstArrayElement = 0;
            imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            imageWrite.descriptorCount = 1;
            imageWrite.pImageInfo = imageInfos.data() + imageInfoIndex;
            descriptorWrites.emplace_back(imageWrite);

            ++binding;
            ++samplerIndex;
            ++imageInfoIndex;
            if (i == 0) {
                m_fixedImages.emplace_back(image);
            }
        }

        samplerIndex = 0;
        for (const auto pair : m_scinthDef->abstract()->drawParameterizedImages()) {
            // Look up default value of parameter using parameter index, provided as second value in the pair.
            size_t parameterIndex = pair.second;
            size_t imageID = static_cast<int>(m_scinthDef->abstract()->parameters()[parameterIndex].defaultValue());
            std::shared_ptr<vk::DeviceImage> image = m_imageMap->getImage(imageID);
            std::shared_ptr<vk::Sampler> sampler = m_scinthDef->parameterizedSamplers()[samplerIndex];
            // We record these decisions on the first run through, for comparison against later parameter updates.
            if (i == 0) {
                m_parameterizedImages.emplace_back(image);
                m_parameterizedImageIDs.emplace_back(std::make_pair(parameterIndex, imageID));
            }

            // Missing images for parameterized images are acceptable, we use the empty image and sampler.
            if (!image) {
                image = m_imageMap->getEmptyImage();
                sampler = m_scinthDef->emptySampler();
            }

            imageInfos[imageInfoIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[imageInfoIndex].imageView = image->view();
            imageInfos[imageInfoIndex].sampler = sampler->get();

            VkWriteDescriptorSet imageWrite = {};
            imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWrite.dstSet = m_descriptorSets[i];
            imageWrite.dstBinding = binding;
            imageWrite.dstArrayElement = 0;
            imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            imageWrite.descriptorCount = 1;
            imageWrite.pImageInfo = imageInfos.data() + imageInfoIndex;
            descriptorWrites.emplace_back(imageWrite);

            ++binding;
            ++samplerIndex;
            ++imageInfoIndex;
        }

        VkDescriptorBufferInfo computeBufferInfo = {};
        if (computeBufferSize) {
            computeBufferInfo.buffer = m_computeBuffers[i]->buffer();
            computeBufferInfo.offset = 0;
            computeBufferInfo.range = m_computeBuffers[i]->size();

            VkWriteDescriptorSet bufferWrite = {};
            bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            bufferWrite.dstSet = m_descriptorSets[i];
            bufferWrite.dstBinding = binding;
            bufferWrite.dstArrayElement = 0;
            bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bufferWrite.descriptorCount = 1;
            bufferWrite.pBufferInfo = &computeBufferInfo;
            bufferWrite.pImageInfo = nullptr;
            bufferWrite.pTexelBufferView = nullptr;
            descriptorWrites.emplace_back(bufferWrite);

            ++binding;
        }

        vkUpdateDescriptorSets(m_device->get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                               0, nullptr);
    }

    return true;
}

void Scinth::updateDescriptors() {
    // This pair is sampler index, new image and we build it by running through parameter values and current bound ids.
    std::vector<std::pair<size_t, std::shared_ptr<vk::DeviceImage>>> newBindings;
    for (size_t i = 0; i < m_parameterizedImageIDs.size(); ++i) {
        size_t parameterIndex = m_parameterizedImageIDs[i].first;
        size_t imageID = static_cast<size_t>(m_parameterValues[parameterIndex]);
        if (imageID != m_parameterizedImageIDs[i].second) {
            std::shared_ptr<vk::DeviceImage> image = m_imageMap->getImage(imageID);
            // Note image can be null, we'll use the empty image in the actual update.
            m_parameterizedImageIDs[i] = std::make_pair(parameterIndex, imageID);
            m_parameterizedImages[i] = image;
            newBindings.emplace_back(std::make_pair(i, image));
        }
    }

    // Early out for no binding updates needed.
    if (!newBindings.size()) {
        return;
    }

    // Compute index where parameterized images binding starts.
    int32_t bindingStart = m_scinthDef->abstract()->uniformManifest().sizeInBytes() ? 1 : 0;
    bindingStart += static_cast<int32_t>(m_scinthDef->abstract()->drawFixedImages().size());

    for (size_t i = 0; i < m_scinthDef->canvas()->numberOfImages(); ++i) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        std::vector<VkDescriptorImageInfo> imageInfos;
        for (auto pair : newBindings) {
            std::shared_ptr<vk::Sampler> sampler = m_scinthDef->parameterizedSamplers()[pair.first];
            std::shared_ptr<vk::DeviceImage> image = pair.second;
            if (!image) {
                sampler = m_scinthDef->emptySampler();
                image = m_imageMap->getEmptyImage();
            }

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = image->view();
            imageInfo.sampler = sampler->get();
            imageInfos.emplace_back(imageInfo);

            VkWriteDescriptorSet imageWrite = {};
            imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWrite.dstSet = m_descriptorSets[i];
            imageWrite.dstBinding = static_cast<uint32_t>(bindingStart + pair.first);
            imageWrite.dstArrayElement = 0;
            imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            imageWrite.descriptorCount = 1;
            imageWrite.pImageInfo = imageInfos.data() + (imageInfos.size() - 1);
            descriptorWrites.emplace_back(imageWrite);
        }

        vkUpdateDescriptorSets(m_device->get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                               0, nullptr);
    }
}

void Scinth::rebuildBuffers() {
    if (m_scinthDef->abstract()->hasComputeStage()) {
        m_computeCommands.reset(new vk::CommandBuffer(m_device, m_scinthDef->computeCommandPool()));
        if (!m_computeCommands->create(m_scinthDef->canvas()->numberOfImages(), false)) {
            spdlog::critical("failed creating compute command buffers for Scinth {}", m_nodeID);
            return;
        }

        for (size_t i = 0; i < m_scinthDef->canvas()->numberOfImages(); ++i) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            VkCommandBufferInheritanceInfo inheritanceInfo = {};
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            beginInfo.pInheritanceInfo = &inheritanceInfo;

            if (vkBeginCommandBuffer(m_computeCommands->buffer(i), &beginInfo) != VK_SUCCESS) {
                spdlog::critical("failed beginning compute command buffer {} for Scinth {}", i, m_nodeID);
                return;
            }

            // If the compute and graphics queues are separate we insert execution barriers around the buffers to
            // prevent simultaneous access to the buffer by both draw and compute shaders.
            if (m_device->computeFamilyIndex() != m_device->graphicsFamilyIndex()) {
                VkBufferMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.srcQueueFamilyIndex = m_device->computeFamilyIndex();
                barrier.dstQueueFamilyIndex = m_device->graphicsFamilyIndex();
                barrier.buffer = m_computeBuffers[i]->buffer();
                barrier.offset = 0;
                barrier.size = m_computeBuffers[i]->size();

                vkCmdPipelineBarrier(m_computeCommands->buffer(i), VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
            }

            // Write parameters as push constants.
            if (m_numberOfParameters) {
                vkCmdPushConstants(
                    m_computeCommands->buffer(i), m_scinthDef->computePipeline()->layout(), VK_SHADER_STAGE_COMPUTE_BIT,
                    0, static_cast<uint32_t>(m_numberOfParameters * sizeof(float)), m_parameterValues.get());
            }

            // Bind compute pipeline.
            vkCmdBindPipeline(m_computeCommands->buffer(i), VK_PIPELINE_BIND_POINT_COMPUTE,
                              m_scinthDef->computePipeline()->get());

            // Bind descriptor sets.
            if (m_descriptorSets.size()) {
                vkCmdBindDescriptorSets(m_computeCommands->buffer(i), VK_PIPELINE_BIND_POINT_COMPUTE,
                                        m_scinthDef->computePipeline()->layout(), 0, 1, &m_descriptorSets[i], 0,
                                        nullptr);
            }

            // Execute compute shader.
            vkCmdDispatch(m_computeCommands->buffer(i), 1, 1, 1);

            // Second memory barrier if needed.
            if (m_device->computeFamilyIndex() != m_device->graphicsFamilyIndex()) {
                VkBufferMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = 0;
                barrier.srcQueueFamilyIndex = m_device->computeFamilyIndex();
                barrier.dstQueueFamilyIndex = m_device->graphicsFamilyIndex();
                barrier.buffer = m_computeBuffers[i]->buffer();
                barrier.offset = 0;
                barrier.size = m_computeBuffers[i]->size();

                vkCmdPipelineBarrier(m_computeCommands->buffer(i), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
            }

            if (vkEndCommandBuffer(m_computeCommands->buffer(i)) != VK_SUCCESS) {
                spdlog::critical("failed ending compute command buffer {} for Scinth {}", i, m_nodeID);
                return;
            }
        }
    }

    m_drawCommands.reset(new vk::CommandBuffer(m_device, m_scinthDef->drawCommandPool()));
    if (!m_drawCommands->create(m_scinthDef->canvas()->numberOfImages(), false)) {
        spdlog::critical("failed creating draw command buffers for Scinth {}", m_nodeID);
        return;
    }

    for (size_t i = 0; i < m_scinthDef->canvas()->numberOfImages(); ++i) {
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

        if (vkBeginCommandBuffer(m_drawCommands->buffer(i), &beginInfo) != VK_SUCCESS) {
            spdlog::critical("failed beginning draw command buffer {} for Scinth {}", i, m_nodeID);
            return;
        }

        if (m_numberOfParameters) {
            vkCmdPushConstants(m_drawCommands->buffer(i), m_scinthDef->drawPipeline()->layout(),
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               static_cast<uint32_t>(m_numberOfParameters * sizeof(float)), m_parameterValues.get());
        }

        vkCmdBindPipeline(m_drawCommands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_scinthDef->drawPipeline()->get());
        VkBuffer vertexBuffers[] = { m_scinthDef->vertexBuffer()->buffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_drawCommands->buffer(i), 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_drawCommands->buffer(i), m_scinthDef->indexBuffer()->buffer(), 0, VK_INDEX_TYPE_UINT16);

        if (m_descriptorSets.size()) {
            vkCmdBindDescriptorSets(m_drawCommands->buffer(i), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_scinthDef->drawPipeline()->layout(), 0, 1, &m_descriptorSets[i], 0, nullptr);
        }

        vkCmdDrawIndexed(m_drawCommands->buffer(i), m_scinthDef->abstract()->shape()->numberOfIndices(), 1, 0, 0, 0);

        if (vkEndCommandBuffer(m_drawCommands->buffer(i)) != VK_SUCCESS) {
            spdlog::critical("failed ending draw command buffer {} for Scinth {}", i, m_nodeID);
            return;
        }
    }

    m_commandBuffersDirty = false;
}

} // namespace comp

} // namespace scin
