#include "vulkan/Pipeline.hpp"

#include "vulkan/Canvas.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/Uniform.hpp"

namespace scin { namespace vk {

Pipeline::Pipeline(std::shared_ptr<Device> device):
    m_device(device),
    m_pipelineLayout(VK_NULL_HANDLE),
    m_pipeline(VK_NULL_HANDLE) {}

Pipeline::~Pipeline() { destroy(); }

bool Pipeline::create(Canvas* canvas, const Manifest& vertexManifest, Shader* vertexShader, Shader* fragmentShader,
        Uniform* uniform) {

    VkVertexInputBindingDescription vertexBindingDescription = {};
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = vertexManifest.sizeInBytes();
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions(vertex_attributes_.size());
    for (size_t i = 0; i < vertex_attributes_.size(); ++i) {
        vertex_attribute_descriptions[i].binding = 0;
        vertex_attribute_descriptions[i].location = i;
        VkFormat format;
        switch (vertex_attributes_[i].first) {
        case kFloat:
            format = VK_FORMAT_R32_SFLOAT;
            break;

        case kVec2:
            format = VK_FORMAT_R32G32_SFLOAT;
            break;

        case kVec3:
            format = VK_FORMAT_R32G32B32_SFLOAT;
            break;

        case kVec4:
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            break;

        }
        vertex_attribute_descriptions[i].format = format;
        vertex_attribute_descriptions[i].offset = vertex_attributes_[i].second;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertexBindingDescription;
    vertex_input_info.vertexAttributeDescriptionCount = vertex_attribute_descriptions.size();
    vertex_input_info.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport State
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain->extent().width);
    viewport.height = static_cast<float>(swapchain->extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain->extent();

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    // Pipeline Layout
    VkPipelineLayoutCreateInfo m_pipelineLayoutinfo = {};
    m_pipelineLayoutinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (uniform) {
        m_pipelineLayoutinfo.setLayoutCount = 1;
        m_pipelineLayoutinfo.pSetLayouts = uniform->pLayout();
    } else {
        m_pipelineLayoutinfo.setLayoutCount = 0;
        m_pipelineLayoutinfo.pSetLayouts = nullptr;
    }
    m_pipelineLayoutinfo.pushConstantRangeCount = 0;
    m_pipelineLayoutinfo.pPushConstantRanges = nullptr;
    if (vkCreatePipelineLayout(device_->get(), &m_pipelineLayoutinfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        return false;
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo vertexStageInfo = {};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = vertex_shader->get();
    vertexStageInfo.pName = vertex_shader->entry_point();

    VkPipelineShaderStageCreateInfo fragmentStageInfo = {};
    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = fragment_shader->get();
    fragmentStageInfo.pName = fragment_shader->entry_point();

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStageInfo, fragmentStageInfo };

    // Pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertex_input_info;
    pipelineInfo.pInputAssemblyState = &input_assembly;
    pipelineInfo.pViewportState = &viewport_state;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &color_blending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = render_pass_; // could get from Swapchain?
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    return (vkCreateGraphicsPipelines(device_->get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_)
            == VK_SUCCESS);
}

void Pipeline::Destroy() {
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_->get(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_->get(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
}


void Pipeline::DestroyPipelineLayout() {
}

} // namespace vk

} // namespace scin
