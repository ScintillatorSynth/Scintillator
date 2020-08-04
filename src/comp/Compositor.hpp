#ifndef SRC_COMP_COMPOSITOR_HPP_
#define SRC_COMP_COMPOSITOR_HPP_

#include "glm/glm.hpp"

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace scin {

namespace audio {
class Ingress;
}

namespace base {
class AbstractScinthDef;
}

namespace vk {
class CommandBuffer;
class CommandPool;
class Device;
class DeviceImage;
class HostBuffer;
}

namespace comp {

class AudioStager;
class Canvas;
class ImageMap;
class SamplerFactory;
class Scinth;
class ScinthDef;
class ShaderCompiler;
class StageManager;

/*! A Compositor keeps the ScinthDef instance dictionary as well as all running Scinths. It can render to a supplied
 * supplied Canvas, which is typically owned by either a Window/SwapChain combination or an Offscreen render pass. The
 * Compositor keeps shared device-specific graphics resources, like a CommandPool and the ShaderCompiler.
 */
class Compositor {
public:
    Compositor(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas);
    ~Compositor();

    bool create();

    /*! Adds a node to the default root blend group at the end of the line, playing after all other nodes. The node
     * will be started on the next call to prepareFrame, and will treat that frameTime as its start time. The named
     * and indexed values are applied to set the initial values of the parameters in the Scinth, otherwise default
     * values as specified in the ScinthDef are used.
     *
     * \param scinthDefName The name of the ScinthDef to invoke.
     * \param nodeID A nodeID for this scinth, if -1 the Compositor will assign a unique negative value.
     * \param namedValues A vector of pairs of parameter names and initial values.
     * \param indexedValues A vector of pairs of parameter indices and initial values.
     * \return True on success, false on error.
     */
    bool cue(const std::string& scinthDefName, int nodeID,
             const std::vector<std::pair<std::string, float>>& namedValues,
             const std::vector<std::pair<int, float>>& indexedValues);

    /*! Stops and removes the nodes from the playing list, and frees the associated resources.
     *
     * \param nodeIDs A list of node IDs to stop and remove from the playing list.
     */
    void freeNodes(const std::vector<int>& nodeIDs);

    /*! Free all nodes within the default group.
     */
    void groupFreeAll();

    /*! Sets the pause/play status of provided nodeID in the provided list of pairs.
     *
     * \param pairs A pair of integers, with the first element as a nodeID and the second as a run value. A value of
     *        zero for the run value will pause the nodeID, and a nonzero value will play it.
     */
    void setRun(const std::vector<std::pair<int, int>>& pairs);


    /*! Unload the shader compiler, releasing the resources associated with it.
     *
     * This can be used to save some memory by releasing the shader compiler, at the cost of increased latency in
     * defining new ScinthDefs, as the compiler will have to be loaded again first.
     */
    void releaseCompiler();

    void destroy();

    void setClearColor(glm::vec3 color) { m_clearColor = color; }

    size_t numberOfRunningScinths();

    /*! Relayed from the graphics device, convenience method.
     */
    bool getGraphicsMemoryBudget(size_t& bytesUsedOut, size_t& bytesBudgetOut);


    std::shared_ptr<StageManager> stageManager() { return m_stageManager; }

private:
    typedef std::list<std::shared_ptr<Scinth>> ScinthList;
    typedef std::unordered_map<int, ScinthList::iterator> ScinthMap;
    typedef std::vector<std::shared_ptr<vk::CommandBuffer>> Commands;
    typedef std::list<std::shared_ptr<AudioStager>> AudioStagerList;

    bool rebuildCommandBuffer();

    /*! Removes a Scinth from playback. Requires that the m_scinthMutex has already been acquired.
     *
     * \param it An iterator from m_scinthMap pointing to the desired Scinth to remove.
     */
    void freeScinthLockAcquired(ScinthMap::iterator it);

    glm::vec3 m_clearColor;

    std::mutex m_scinthDefMutex;
    std::unordered_map<std::string, std::shared_ptr<ScinthDef>> m_scinthDefs;

    // Protects m_scinths, m_scinthMap, m_audioStagers
    std::mutex m_scinthMutex;
    // A list, in order of evaluation, of all currently running Scinths.
    ScinthList m_scinths;
    // A map from Scinth instance names to elements in the running instance list.
    ScinthMap m_scinthMap;
    // A list of the audio stage objects to update each frame.
    AudioStagerList m_audioStagers;

    // Following should only be accessed from the same thread that calls prepareFrame.
    std::shared_ptr<vk::CommandBuffer> m_drawPrimary;
    // We keep the subcommand buffers referenced each frame, and make a copy of them at each image index, so that they
    // are always valid until we are rendering a new frame over the old commands.
    Commands m_drawSecondary;

    std::shared_ptr<vk::CommandBuffer> m_computePrimary;
    Commands m_computeSecondary;

    std::vector<Commands> m_computeCommands;
    std::vector<Commands> m_drawCommands;
};

} // namespace comp

} // namespace scin

#endif // SRC_COMP_COMPOSITOR_HPP_
