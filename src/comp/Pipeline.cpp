#include "comp/Pipeline.hpp"

#include "base/Shape.hpp"
#include "comp/Canvas.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Shader.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

Pipeline::Pipeline(std::shared_ptr<vk::Device> device):
    m_device(device),
    m_pipelineLayout(VK_NULL_HANDLE),
    m_pipeline(VK_NULL_HANDLE) {}

Pipeline::~Pipeline() { destroy(); }

bool Pipeline::create(const base::Manifest& vertexManifest, const base::Shape* shape, Canvas* canvas,
                      std::shared_ptr<vk::Shader> vertexShader, std::shared_ptr<vk::Shader> fragmentShader,
                      VkDescriptorSetLayout descriptorSetLayout, size_t pushConstantBlockSize,
                      const base::RenderOptions& renderOptions) {
    // Keep references to these graphics objects so they won't be destroyed until after this Pipeline is destroyed.
    m_vertexShader = vertexShader;
    m_fragmentShader = fragmentShader;

    VkVertexInputBindingDescription vertexBindingDescription = {};
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = vertexManifest.sizeInBytes();
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> vertexAttributes(vertexManifest.numberOfElements());
    for (auto i = 0; i < vertexManifest.numberOfElements(); ++i) {
        vertexAttributes[i].binding = 0;
        vertexAttributes[i].location = i;
        VkFormat format;
        switch (vertexManifest.typeForElement(i)) {
        case base::Manifest::ElementType::kFloat:
            format = VK_FORMAT_R32_SFLOAT;
            break;

        case base::Manifest::ElementType::kVec2:
            format = VK_FORMAT_R32G32_SFLOAT;
            break;

        case base::Manifest::ElementType::kVec3:
            format = VK_FORMAT_R32G32B32_SFLOAT;
            break;

        case base::Manifest::ElementType::kVec4:
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            break;
        }
        vertexAttributes[i].format = format;
        vertexAttributes[i].offset = vertexManifest.offsetForElement(i);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributes.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    switch (shape->topology()) {
    case base::Shape::Topology::kTriangleStrip:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        break;

    default:
        spdlog::error("Unsupported topology in shape.");
        return false;
    }
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport State
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(canvas->width());
    viewport.height = static_cast<float>(canvas->height());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = canvas->extent();

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    switch (renderOptions.polygonMode()) {
    case base::RenderOptions::PolygonMode::kFill:
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        break;
    case base::RenderOptions::PolygonMode::kLine:
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        break;

    case base::RenderOptions::PolygonMode::kPoint:
        rasterizer.polygonMode = VK_POLYGON_MODE_POINT;
        break;
    }
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkDescriptorSetLayout setLayouts[] = { descriptorSetLayout };
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = setLayouts;
    } else {
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
    }
    VkPushConstantRange pushConstantRange = {};
    if (pushConstantBlockSize) {
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pushConstantBlockSize;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    } else {
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
    }

    if (vkCreatePipelineLayout(m_device->get(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        return false;
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo vertexStageInfo = {};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = vertexShader->get();
    vertexStageInfo.pName = vertexShader->entryPoint();

    VkPipelineShaderStageCreateInfo fragmentStageInfo = {};
    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = fragmentShader->get();
    fragmentStageInfo.pName = fragmentShader->entryPoint();

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStageInfo, fragmentStageInfo };

    // Pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = canvas->renderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    return (vkCreateGraphicsPipelines(m_device->get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline)
            == VK_SUCCESS);
}

void Pipeline::destroy() {
    spdlog::debug("Pipeline destructor");
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device->get(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device->get(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
}

} // namespace vk

} // namespace scin
