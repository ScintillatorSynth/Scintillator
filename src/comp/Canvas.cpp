#include "comp/Canvas.hpp"

#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

Canvas::Canvas(std::shared_ptr<vk::Device> device): m_device(device), m_renderPass(VK_NULL_HANDLE) {}

Canvas::~Canvas() { destroy(); }

bool Canvas::create(const std::vector<std::shared_ptr<vk::Image>>& images) {
    m_numberOfImages = images.size();
    if (m_numberOfImages == 0) {
        spdlog::error("Canvas failed to build from empty set of images.");
        return false;
    }

    m_extent = images[0]->extent();

    for (auto image : images) {
        if (!image->createView()) {
            spdlog::error("Canvas failed to create framebuffer image views.");
            return false;
        }
    }

    // Next is the render pass.
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = images[0]->format();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1; // TODO: alpha-blended Scinths might need their own subpasses?
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device->get(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        spdlog::error("error creating render pass");
        return false;
    }

    // Lastly create the Framebuffers, one per image.
    m_framebuffers.resize(m_numberOfImages);
    for (auto i = 0; i < m_numberOfImages; ++i) {
        VkImageView attachments[] = { images[i]->view() };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_extent.width;
        framebufferInfo.height = m_extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device->get(), &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            spdlog::error("error creating framebuffers");
            return false;
        }
    }

    if (m_extent.width > m_extent.height) {
        m_normPosScale.x = static_cast<float>(m_extent.width) / static_cast<float>(m_extent.height);
        m_normPosScale.y = 1.0f;
    } else {
        m_normPosScale.x = 1.0f;
        m_normPosScale.y = static_cast<float>(m_extent.height) / static_cast<float>(m_extent.width);
    }

    return true;
}

void Canvas::destroy() {
    for (auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(m_device->get(), framebuffer, nullptr);
    }
    m_framebuffers.clear();

    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device->get(), m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
}

} // namespace vk

} // namespace scin
