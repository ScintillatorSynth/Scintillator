#include "comp/ScinthDef.hpp"

#include "base/AbstractScinthDef.hpp"
#include "base/Intrinsic.hpp"
#include "base/Shape.hpp"
#include "base/VGen.hpp"
#include "comp/Canvas.hpp"
#include "comp/Pipeline.hpp"
#include "comp/SamplerFactory.hpp"
#include "comp/Scinth.hpp"
#include "comp/ShaderCompiler.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Device.hpp"

#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include <array>

namespace scin { namespace comp {

ScinthDef::ScinthDef(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas,
                     std::shared_ptr<vk::CommandPool> computeCommandPool,
                     std::shared_ptr<vk::CommandPool> drawCommandPool, std::shared_ptr<SamplerFactory> samplerFactory,
                     std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef):
    m_device(device),
    m_canvas(canvas),
    m_computeCommandPool(computeCommandPool),
    m_drawCommandPool(drawCommandPool),
    m_samplerFactory(samplerFactory),
    m_abstract(abstractScinthDef),
    m_descriptorSetLayout(VK_NULL_HANDLE) {}

ScinthDef::~ScinthDef() {
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device->get(), m_descriptorSetLayout, nullptr);
    }

    for (auto sampler : m_fixedSamplers) {
        m_samplerFactory->releaseSampler(sampler);
    }
    for (auto sampler : m_parameterizedSamplers) {
        m_samplerFactory->releaseSampler(sampler);
    }
    if (m_emptySampler) {
        m_samplerFactory->releaseSampler(m_emptySampler);
    }
}

bool ScinthDef::build(ShaderCompiler* compiler) {
    // Build the vertex data. Because Intrinsics can add data payloads to the vertex data, each ScinthDef shares a
    // vertex buffer and index buffer across all Scinth instances, allowing for the potential unique combination
    // between Shape data and payloads.
    if (!buildVertexData()) {
        spdlog::error("error building vertex data for ScinthDef {}.", m_abstract->name());
        return false;
    }

    m_vertexShader = compiler->compile(m_device, m_abstract->vertexShader(), m_abstract->prefix() + "_vertexShader",
                                       "main", vk::Shader::kVertex);
    if (!m_vertexShader) {
        spdlog::error("error compiling vertex shader for ScinthDef {}.", m_abstract->name());
        return false;
    }

    m_fragmentShader = compiler->compile(m_device, m_abstract->fragmentShader(),
                                         m_abstract->prefix() + "_fragmentShader", "main", vk::Shader::kFragment);
    if (!m_fragmentShader) {
        spdlog::error("error compiling fragment shader for ScinthDef {}", m_abstract->name());
        return false;
    }

    if (!buildDescriptorLayout()) {
        spdlog::error("ScinthDef failed to build descriptor layout for {}", m_abstract->name());
        return false;
    }

    if (!buildSamplers()) {
        spdlog::error("ScinthDef failed to build Samplers list for ScinthDef {}", m_abstract->name());
        return false;
    }

    m_drawPipeline.reset(new Pipeline(m_device));
    if (!m_drawPipeline->create(m_abstract->vertexManifest(), m_abstract->shape(), m_canvas.get(), m_vertexShader,
                            m_fragmentShader, m_descriptorSetLayout, m_abstract->parameters().size() * sizeof(float),
                            m_abstract->renderOptions())) {
        spdlog::error("Error creating draw pipeline for ScinthDef {}", m_abstract->name());
        return false;
    }

    if (m_abstract->hasComputeStage()) {
        m_computeShader = compiler->compile(m_device, m_abstract->computeShader(),
                m_abstract->prefix() + "_computeShader", "main", vk::Shader::kCompute);
        if (!m_computeShader) {
            spdlog::error("error compiling compute shader for ScinthDef {}.", m_abstract->name());
            return false;
        }
        m_computePipeline.reset(new Pipeline(m_device));
        if (!m_computePipeline->createCompute(m_computeShader, m_descriptorSetLayout, m_abstract->parameters().size())) {
            spdlog::error("Error creating compute pipeline for ScinthDef {}", m_abstract->name());
            return false;
        }
    }

    return true;
}

bool ScinthDef::buildVertexData() {
    // Build the vertex data based on the manifest and the shape.
    size_t numberOfFloats =
        m_abstract->shape()->numberOfVertices() * (m_abstract->vertexManifest().sizeInBytes() / sizeof(float));
    m_vertexBuffer.reset(new vk::HostBuffer(m_device, vk::Buffer::Kind::kVertex, numberOfFloats * sizeof(float)));
    if (!m_vertexBuffer->create()) {
        spdlog::error("Error creating vertex buffer for ScinthDef {}", m_abstract->name());
        return false;
    }
    spdlog::info("Copying {} bytes of vertex data to GPU for ScinthDef {}", m_vertexBuffer->size(), m_abstract->name());
    if (!m_abstract->shape()->storeVertexData(m_abstract->vertexManifest(), m_canvas->normPosScale(),
                                              static_cast<float*>(m_vertexBuffer->mappedAddress()))) {
        spdlog::error("Failed to build Shape vertex data in ScinthDef {}", m_abstract->name());
        return false;
    }

    // Lastly, copy the index buffer as well.
    m_indexBuffer.reset(new vk::HostBuffer(m_device, vk::Buffer::Kind::kIndex,
                                           m_abstract->shape()->numberOfIndices() * sizeof(uint16_t)));
    if (!m_indexBuffer->create()) {
        spdlog::error("error creating index buffer for ScinthDef {}", m_abstract->name());
        return false;
    }
    spdlog::info("copying {} bytes of index data to GPU for ScinthDef {}", m_indexBuffer->size(), m_abstract->name());
    if (!m_abstract->shape()->storeIndexData(static_cast<uint16_t*>(m_indexBuffer->mappedAddress()))) {
        spdlog::error("Failed to build Shape index data in ScinthDef {}", m_abstract->name());
        return false;
    }

    return true;
}

bool ScinthDef::buildDescriptorLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    if (m_abstract->uniformManifest().sizeInBytes()) {
        VkDescriptorSetLayoutBinding uniformBinding = {};
        uniformBinding.binding = bindings.size();
        uniformBinding.descriptorCount = 1;
        uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.emplace_back(uniformBinding);
    }

    auto totalSamplers = m_abstract->drawFixedImages().size() + m_abstract->drawParameterizedImages().size();
    for (auto i = 0; i < totalSamplers; ++i) {
        VkDescriptorSetLayoutBinding imageBinding = {};
        imageBinding.binding = bindings.size();
        imageBinding.descriptorCount = 1;
        imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.emplace_back(imageBinding);
    }

    if (m_abstract->computeManifest().sizeInBytes()) {
        VkDescriptorSetLayoutBinding bufferBinding = {};
        bufferBinding.binding = bindings.size();
        bufferBinding.descriptorCount = 1;
        bufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.emplace_back(bufferBinding);
    }

    if (bindings.size()) {
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(m_device->get(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool ScinthDef::buildSamplers() {
    for (auto pair : m_abstract->drawFixedImages()) {
        std::shared_ptr<vk::Sampler> sampler = m_samplerFactory->getSampler(pair.first);
        if (!sampler) {
            spdlog::error("ScinthDef {} did not find fixed Sampler with key {:08x}", m_abstract->name(), pair.first);
            return false;
        }

        m_fixedSamplers.emplace_back(sampler);
    }

    for (auto pair : m_abstract->drawParameterizedImages()) {
        std::shared_ptr<vk::Sampler> sampler = m_samplerFactory->getSampler(pair.first);
        if (!sampler) {
            spdlog::error("ScinthDef {} did not find parameterized Sampler with key {:08x}", m_abstract->name(),
                          pair.first);
            return false;
        }

        m_parameterizedSamplers.emplace_back(sampler);
    }

    // If there are parameterized images we will want to create the empty sampler, to ensure that empty images render
    // correctly as transparent black.
    if (m_parameterizedSamplers.size()) {
        m_emptySampler = m_samplerFactory->getSampler(0);
    }

    return true;
}

} // namespace comp

} // namespace scin
