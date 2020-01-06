#ifndef SRC_COMPOSITOR_HPP_
#define SRC_COMPOSITOR_HPP_

#include "core/Types.hpp"

#include "glm/glm.hpp"

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace scin {

namespace vk {
class Canvas;
class CommandBuffer;
class CommandPool;
class Device;
class ShaderCompiler;
} // namespace vk

class AbstractScinthDef;
class Scinth;
class ScinthDef;

/*! A Compositor keeps the ScinthDef instance dictionary as well as all running Scinths. It can render to a supplied
 * supplied Canvas, which is typically owned by either a Window/SwapChain combination or an Offscreen render pass. The
 * Compositor keeps shared device-specific graphics resources, like a CommandPool and the ShaderCompiler.
 */
class Compositor {
public:
    Compositor(std::shared_ptr<vk::Device> device, std::shared_ptr<vk::Canvas> canvas);
    ~Compositor();

    bool create();

    bool buildScinthDef(std::shared_ptr<const AbstractScinthDef> abstractScinthDef);

    /*! Adds a node to the default root blend group at the end of the line, playing after all other nodes.
     *
     * \param scinthDefName The name of the ScinthDef to invoke.
     * \param nodeID A nodeID for this scinth, if -1 the Compositor will assign a unique negative value.
     * \startTime The start time to consider this Scinth started at.
     * \return True on success, false on error.
     */
    bool play(const std::string& scinthDefName, int nodeID, const TimePoint& startTime);

    /*! Stops and removes the nodes from the playing list, and frees the associated resources.
     *
     * \param nodeIDs A list of node IDs to stop and remove from the playing list.
     */
    void freeNodes(const std::vector<int>& nodeIDs);

    /*! Sets the pause/play status of provided nodeID in the provided list of pairs.
     *
     * \param pairs A pair of integers, with the first element as a nodeID and the second as a run value. A value of
     *        zero for the run value will pause the nodeID, and a nonzero value will play it.
     */
    void setRun(const std::vector<std::pair<int, int>>& pairs);

    /*! Prepare and return a CommandBuffers that when executed in order will render the current frame.
     *
     * \param imageIndex The index of the imageView in the Canvas we will be rendering in to.
     * \param frameTime The point in time at which to build this frame for.
     * \return A CommandBuffer object to be scheduled for graphics queue submission.
     */
    std::shared_ptr<vk::CommandBuffer> prepareFrame(uint32_t imageIndex, const TimePoint& frameTime);

    /*! Unload the shader compiler, releasing the resources associated with it.
     *
     * This can be used to save some memory by releasing the shader compiler, at the cost of increased latency in
     * defining new ScinthDefs, as the compiler will have to be loaded again first.
     */
    void releaseCompiler();

    void destroy();

    void setClearColor(glm::vec3 color) { m_clearColor = color; }

private:
    typedef std::list<std::shared_ptr<Scinth>> ScinthList;
    typedef std::unordered_map<int, ScinthList::iterator> ScinthMap;
    typedef std::vector<std::shared_ptr<vk::CommandBuffer>> Commands;

    bool rebuildCommandBuffer();

    /*! Removes a Scinth from playback. Requires that the m_scinthMutex has already been acquired.
     *
     * \param it An iterator from m_scinthMap pointing to the desired Scinth to remove.
     */
    void freeScinthLockAcquired(ScinthMap::iterator it);

    std::shared_ptr<vk::Device> m_device;
    std::shared_ptr<vk::Canvas> m_canvas;
    glm::vec3 m_clearColor;

    std::unique_ptr<vk::ShaderCompiler> m_shaderCompiler;
    std::shared_ptr<vk::CommandPool> m_commandPool;
    std::atomic<bool> m_commandBufferDirty;
    std::atomic<int> m_nodeSerial;

    std::mutex m_scinthDefMutex;
    std::unordered_map<std::string, std::shared_ptr<ScinthDef>> m_scinthDefs;

    // Protects m_scinths, m_scinthMap.
    std::mutex m_scinthMutex;
    // A list, in order of evaluation, of all currently running Scinths.
    ScinthList m_scinths;
    // A map from Scinth instance names to elements in the running instance list.
    ScinthMap m_scinthMap;

    // Following should only be accessed from the same thread that calls prepareFrame.
    std::shared_ptr<vk::CommandBuffer> m_primaryCommands;
    // We keep the subcommand buffers referenced each frame, and make a copy of them at each image index, so that they
    // are always valid until we are rendering a new frame over the old commands.
    Commands m_secondaryCommands;
    std::vector<Commands> m_frameCommands;
};

} // namespace scin

#endif // SRC_COMPOSITOR_HPP_
