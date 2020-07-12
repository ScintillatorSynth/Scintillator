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

    /*! Construct a ScinthDef designed to render into this Compositor and add to the local ScinthDef map.
     *
     * \param abstractScinthDef The template to build the ScinthDef from
     * \return true on success, false on failure.
     */
    bool buildScinthDef(std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef);

    /*! Remove the supplied ScinthDefs from this Compositor's map.
     *
     * \param names A list of ScinthDef names to remove.
     */
    void freeScinthDefs(const std::vector<std::string>& names);

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

    /*! Sets the pause/play status of provided nodeID in the provided list of pairs.
     *
     * \param pairs A pair of integers, with the first element as a nodeID and the second as a run value. A value of
     *        zero for the run value will pause the nodeID, and a nonzero value will play it.
     */
    void setRun(const std::vector<std::pair<int, int>>& pairs);

    /*! Change the parameter values for a node.
     *
     * \param nodeID The ID of the node to modify.
     * \param namedValues A vector of pairs of parameter names and new values.
     * \param indexedValues A vector of pairs of parameter indexes and new values.
     */
    void setNodeParameters(int nodeID, const std::vector<std::pair<std::string, float>>& namedValues,
                           const std::vector<std::pair<int, float>> indexedValues);

    /*! Prepare the CommandBuffers that when executed in order will render the current frame. Updates the values
     * returned by computeCommands() and drawCommands().
     *
     * \param imageIndex The index of the imageView in the Canvas we will be rendering in to.
     * \param frameTime The point in time at which to build this frame for.
     * \return A boolean indicating success in frame preparation.
     */
    bool prepareFrame(uint32_t imageIndex, double frameTime);

    std::shared_ptr<vk::CommandBuffer> computeCommands() { return m_computePrimary; }
    std::shared_ptr<vk::CommandBuffer> drawCommands() { return m_drawPrimary; }

    /*! Unload the shader compiler, releasing the resources associated with it.
     *
     * This can be used to save some memory by releasing the shader compiler, at the cost of increased latency in
     * defining new ScinthDefs, as the compiler will have to be loaded again first.
     */
    void releaseCompiler();

    void destroy();

    void setClearColor(glm::vec3 color) { m_clearColor = color; }

    int numberOfRunningScinths();

    /*! Relayed from the graphics device, convenience method.
     */
    bool getGraphicsMemoryBudget(size_t& bytesUsedOut, size_t& bytesBudgetOut);

    /*! Prepare to copy the provided decoded image to a suitable GPU-local buffer.
     *
     * \param imageID The integer image identifier for this
     * \param width The width of the image in pixels.
     * \param height The height of the image in pixels.
     * \param imageBuffer The bytes of the image in RGBA format.
     * \param completion A function to call once the image has been staged.
     */
    void stageImage(int imageID, int width, int height, std::shared_ptr<vk::HostBuffer> imageBuffer,
                    std::function<void()> completion);

    /*! Returns basic information about a compositor image associated with imageID, if it exists.
     *
     * \param imageID The image ID to query
     * \param sizeOut Will store the size in bytes of the image buffer
     * \param widthOut Will store the width of the image buffer in pixels
     * \param heightOut Will store the height of the image buffer in pixels
     * \return True if the image was found and the output values have been written, false otherwise.
     */
    bool queryImage(int imageID, int& sizeOut, int& widthOut, int& heightOut);

    /*! Adds an audio Ingress object for provision of audio data to the GPU.
     *
     * \param ingress The ingress object to consume audio from. Compositor will attempt to extract audio data from the
     *        source on every call to prepareFrame().
     * \param imageID The image ID to associate with this audio stream, for sampling.
     * \return true on success, false on failure.
     */
    bool addAudioIngress(std::shared_ptr<audio::Ingress> ingress, int imageID);

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

    std::shared_ptr<vk::Device> m_device;
    std::shared_ptr<Canvas> m_canvas;
    glm::vec3 m_clearColor;

    std::unique_ptr<ShaderCompiler> m_shaderCompiler;
    std::shared_ptr<vk::CommandPool> m_computeCommandPool;
    std::shared_ptr<vk::CommandPool> m_drawCommandPool;
    std::shared_ptr<StageManager> m_stageManager;
    std::shared_ptr<SamplerFactory> m_samplerFactory;
    std::shared_ptr<ImageMap> m_imageMap;
    std::atomic<bool> m_commandBufferDirty;
    std::atomic<int> m_nodeSerial;

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
