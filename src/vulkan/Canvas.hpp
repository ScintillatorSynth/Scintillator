#ifndef SRC_VULKAN_CANVAS_HPP_
#define SRC_VULKAN_CANVAS_HPP_

#include "vulkan/Vulkan.hpp"

#include <vector>

namespace scin {

namespace vk {

/*! Contains the data and state required to render to a set of Vulkan Images.
 *
 * Can be provided to the Compositor as either an output target or an intermediate target for rendering.
 */
class Canvas {
public:
    Canvas(size_t width, size_t height);
    ~Canvas();

    /*! Makes a set of ImageViews, a Render Pass, and then a set of Framebuffers.
     *
     * \param images An array of one or more images to make corresponding ImageViews and FrameBuffers for.
     * \param format The format of the provided images.
     */
    bool create(const std::vector<VkImage> images, VkFormat format); // TODO: consider abstracting to like an ImageSet
                                                                     // or something? ImageSet could know its own width,
                                                                     // height, and format.
private:
    std::vector<VkImageView> m_imageViews;
    std::vector<VkFramebuffer> m_framebuffers;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_CANVAS_HPP_
