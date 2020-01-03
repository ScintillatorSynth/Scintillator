#include "vulkan/Canvas.hpp"

#include "vulkan/Device.hpp"
#include "vulkan/Images.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Canvas::Canvas(std::shared_ptr<Device> device): m_device(device), m_renderPass(VK_NULL_HANDLE) {}

Canvas::~Canvas() { destroy(); }

bool Canvas::create(std::shared_ptr<Images> images) {
    m_images = images;
    m_extent = images->extent();
    m_numberOfImages = images->count();

    // Create vkImageViews, one per image.
    m_imageViews.resize(m_numberOfImages);
    for (auto i = 0; i < m_numberOfImages; ++i) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = images->get()[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = images->format();
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(m_device->get(), &viewCreateInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS) {
            spdlog::error("failed to create image view.");
            return false;
        }
    }

    // Next is the render pass.
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = images->format();
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
        VkImageView attachments[] = { m_imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = images->width();
        framebufferInfo.height = images->height();
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device->get(), &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            spdlog::error("error creating framebuffers");
            return false;
        }
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

    for (auto view : m_imageViews) {
        vkDestroyImageView(m_device->get(), view, nullptr);
    }
    m_imageViews.clear();
}

VkImage Canvas::image(size_t index) { return m_images->get()[index]; }

} // namespace vk

} // namespace scin
