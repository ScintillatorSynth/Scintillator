#include "vulkan/Swapchain.hpp"

#include "vulkan/Canvas.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Images.hpp"
#include "vulkan/Pipeline.hpp"
#include "vulkan/Window.hpp"

#include "spdlog/spdlog.h"

#include <limits>
#include <vector>

namespace scin { namespace vk {

Swapchain::Swapchain(std::shared_ptr<Device> device):
    m_device(device),
    m_imageCount(0),
    m_swapchain(VK_NULL_HANDLE),
    m_images(new Images(device)),
    m_canvas(new Canvas(device)) {}

Swapchain::~Swapchain() { destroy(); }

bool Swapchain::create(Window* window) {
    // Pick swap chain format from available options.
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->getPhysical(), window->getSurface(), &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->getPhysical(), window->getSurface(), &formatCount, formats.data());
    // If the only entry returned is UNDEFINED that means the surface supports all formats equally, pick preferred
    // BGRA format.
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        m_surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    } else {
        // Pick first one by default, but go through list of formats to see if
        // our preferred option is available and we can use that instead.
        m_surfaceFormat = formats[0];
        for (const auto& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_surfaceFormat = format;
                break;
            }
        }
    }

    // Pick present mode, with preference for MAILBOX if supported, which allows for lower-latency renders.
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_device->getPhysical(), window->getSurface(), &presentModeCount,
                                              nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_device->getPhysical(), window->getSurface(), &presentModeCount,
                                              presentModes.data());
    m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            m_presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // Choose swap extent, pixel dimensions of swap chain.
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device->getPhysical(), window->getSurface(), &capabilities);
    // If the currentExtent field is set to MAX_UINT this means we can pick the size of the extent we want, otherwise
    // we should use the size provided in the capabilities struct.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        m_extent = capabilities.currentExtent;
    } else {
        m_extent = { static_cast<uint32_t>(window->width()), static_cast<uint32_t>(window->height()) };
        m_extent.width = std::max(capabilities.minImageExtent.width,
                                  std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(window->width())));
        m_extent.height =
            std::max(capabilities.minImageExtent.height,
                     std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(window->height())));
    }

    m_imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        m_imageCount = std::min(m_imageCount, capabilities.maxImageCount);
    }

    // Populate Swap Chain create info structure.
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = window->getSurface();
    createInfo.minImageCount = m_imageCount;
    createInfo.imageFormat = m_surfaceFormat.format;
    createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
    createInfo.imageExtent = m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(m_device->graphicsFamilyIndex()),
                                      static_cast<uint32_t>(m_device->presentFamilyIndex()) };

    if (m_device->graphicsFamilyIndex() != m_device->presentFamilyIndex()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = m_presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device->get(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        spdlog::error("error creating swap chain");
        return false;
    }

    m_imageCount = m_images->getFromSwapchain(this, m_imageCount);
    if (!m_canvas->create(m_images.get())) {
        spdlog::error("error creating Canvas");
        return false;
    }

    return true;
}

void Swapchain::destroy() {
    m_canvas->destroy();

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device->get(), m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

} // namespace vk

} // namespace scin
