#include "vulkan/Swapchain.hpp"

#include "vulkan/Device.hpp"
#include "vulkan/Pipeline.hpp"
#include "vulkan/Window.hpp"

#include <iostream>
#include <limits>
#include <vector>

namespace scin { namespace vk {

Swapchain::Swapchain(std::shared_ptr<Device> device): device_(device), image_count_(0), swapchain_(VK_NULL_HANDLE) {}

Swapchain::~Swapchain() { Destroy(); }

bool Swapchain::Create(Window* window) {
    // Pick swap chain format from available options.
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device_->get_physical(), window->get_surface(), &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device_->get_physical(), window->get_surface(), &format_count, formats.data());
    // If the only entry returned is UNDEFINED that means the surface supports
    // all formats equally, pick preferred BGRA format.
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        surface_format_ = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    } else {
        // Pick first one by default, but go through list of formats to see if
        // our preferred option is available and we can use that instead.
        surface_format_ = formats[0];
        for (const auto& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surface_format_ = format;
                break;
            }
        }
    }

    // Pick present mode, with preference for MAILBOX if supported, which
    // allows for lower-latency renders.
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device_->get_physical(), window->get_surface(), &present_mode_count,
                                              nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device_->get_physical(), window->get_surface(), &present_mode_count,
                                              present_modes.data());
    present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& mode : present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode_ = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // Choose swap extent, pixel dimensions of swap chain.
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_->get_physical(), window->get_surface(), &capabilities);
    // If the currentExtent field is set to MAX_UINT this means we can pick
    // the size of the extent we want, otherwise we should use the size
    // provided in the capabilities struct.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent_ = capabilities.currentExtent;
    } else {
        extent_ = { static_cast<uint32_t>(window->width()), static_cast<uint32_t>(window->height()) };
        extent_.width = std::max(capabilities.minImageExtent.width,
                                 std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(window->width())));
        extent_.height =
            std::max(capabilities.minImageExtent.height,
                     std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(window->height())));
    }

    image_count_ = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        image_count_ = std::min(image_count_, capabilities.maxImageCount);
    }

    // Populate Swap Chain create info structure.
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = window->get_surface();
    create_info.minImageCount = image_count_;
    create_info.imageFormat = surface_format_.format;
    create_info.imageColorSpace = surface_format_.colorSpace;
    create_info.imageExtent = extent_;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_family_indices[] = { static_cast<uint32_t>(device_->graphics_family_index()),
                                        static_cast<uint32_t>(device_->present_family_index()) };

    if (device_->graphics_family_index() != device_->present_family_index()) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode_;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_->get(), &create_info, nullptr, &swapchain_) != VK_SUCCESS) {
        std::cerr << "error creating swap chain!" << std::endl;
        return false;
    }

    // Retrieve images from swap chain. Note it may be possible that Vulkan
    // has allocated more images than requested by the create call, so we
    // query first for the actual image count.
    vkGetSwapchainImagesKHR(device_->get(), swapchain_, &image_count_, nullptr);
    images_.resize(image_count_);
    vkGetSwapchainImagesKHR(device_->get(), swapchain_, &image_count_, images_.data());

    // Create image views to access each image.
    image_views_.resize(image_count_);
    for (size_t i = 0; i < images_.size(); ++i) {
        VkImageViewCreateInfo view_create_info = {};
        view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_create_info.image = images_[i];
        view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_create_info.format = surface_format_.format;
        view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_create_info.subresourceRange.baseMipLevel = 0;
        view_create_info.subresourceRange.levelCount = 1;
        view_create_info.subresourceRange.baseArrayLayer = 0;
        view_create_info.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device_->get(), &view_create_info, nullptr, &image_views_[i]) != VK_SUCCESS) {
            std::cerr << "failed to create image view." << std::endl;
            return false;
        }
    }

    return true;
}

void Swapchain::Destroy() {
    DestroyFramebuffers();

    for (auto view : image_views_) {
        vkDestroyImageView(device_->get(), view, nullptr);
    }
    image_views_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_->get(), swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

bool Swapchain::CreateFramebuffers(Pipeline* pipeline) {
    framebuffers_.resize(image_count_);

    for (size_t i = 0; i < image_views_.size(); ++i) {
        VkImageView attachments[] = { image_views_[i] };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = pipeline->render_pass();
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = extent_.width;
        framebuffer_info.height = extent_.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(device_->get(), &framebuffer_info, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

void Swapchain::DestroyFramebuffers() {
    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device_->get(), framebuffer, nullptr);
    }

    framebuffers_.clear();
}

} // namespace vk

} // namespace scin
