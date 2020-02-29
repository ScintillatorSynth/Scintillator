#ifndef SRC_VULKAN_STAGE_MANAGER_HPP_
#    define SRC_VULKAN_STAGE_MANAGER_HPP_

namespace scin { namespace vk {

/*! On Discrete GPU devices at least, if not other classes of devices, copying data from host-accessible buffers to
 * GPU-only accessible memory may result in a performance improvement. As Scinths, ScinthDefs, and the general
 * Compositor may all want to do this, the StageManager centralizes management of those staging requests so they can be
 * batched and processed all at once.
 *
 * It also provides a fence for all batched staging requests, and a thread that blocks on that fence, to wait to
 * process callbacks until after content has been staged.
 *
 */
class StageManager {};

} // namespace vk

} // namespace scin
