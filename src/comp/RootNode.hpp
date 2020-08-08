#ifndef SRC_COMP_ROOT_NODE_HPP_
#define SRC_COMP_ROOT_NODE_HPP_

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
class HostBuffer;
}

namespace comp {

class AudioStager;
class Canvas;
class FrameContext;
class ImageMap;
class SamplerFactory;
class Scinth;
class ScinthDef;
class ShaderCompiler;
class StageManager;

/*
 * Things RootNode needs to be able to do:
 * a) quickly know the number of group and running scinths
 * b) quickly find groups and scinths by nodeID
 * c) delete only the scinths in a subTree
 * d) recursively delete everything in a subTree
 * e) move groups and scinths arbitrarily within the tree
 * f) quickly know if a given node is a group or a scinth
 *
 * Since at the end of the day the list needs to be completely flat, what if the groups were just an ordered pair
 * start/end of std::list<std::shared_ptr<Scinth>> iterators - we could get rid of this polymorphism nonsense and just
 * emulate it all with a few internal data structures here in rootNode. Kind of a fun full-circle programming gig where
 * we built the whole tree then just decided the first idea was the best - of a flat list of stuff. Groups then have
 * a start iterator, end iterator (which is one past the end), and a count (because you don't know how many there might
 * be). They are kept in their own unordered_map<int, Group> for quick access. There's some care required to make sure
 * that Groups don't overlap in the flat list of Scinths. But that should be fine. And the moving of a group or a
 * Scinth becomes different depending on which one is moving. Ah but that's the rub - if there's a subgroup do those
 * iterators get invalidated? No I say! Because iterators aren't themselves elements in the list. Ooh clever.
 */

/*! Root object of the render tree. Maintains global objects for the render tree such as currently defined ScinthDefs
 * and image buffers. Creates the primary command buffers and render passes. Renders to a Canvas. Maintains a hard-coded
 * nodeID of 0. Maintains the render tree internally as a flat list of Scinths and a set of Group data structures.
 */
class RootNode {
public:
    /*! Construct a root node. Root nodes assume a hard-coded nodeID of 0.
     *
     * \param device The Vulkan device.
     * \param canvas The canvas that root node will render all contained nodes to for its output render pass.
     */
    RootNode(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas);
    virtual ~RootNode() = default;

    /*! Creates the root node, including setting up much of the shared resources like the empty image.
     *
     * \return true on success, false on failure.
     */
    bool create();

    /*! In this case returns true if the primary command buffer had to be rebuilt, which can be useful for tracking
     * statistics about effectiveness of caching command buffers.
     */
    bool prepareFrame(std::shared_ptr<FrameContext> context);

    void destroy();

    //** methods supporting OSC commands
    enum AddAction : int32_t {
        kGroupHead = 0,
        kGroupTail = 1,
        kBeforeNode = 2,
        kAfterNode = 3,
        kReplace = 4,
        kActionCount = 5
    };

    /*! Construct a ScinthDef designed to render into this RootNode and add to the local ScinthDef map.
     *
     * \param abstractScinthDef The template to build the ScinthDef from
     * \return true on success, false on failure.
     */
    bool defAdd(std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef);

    /*! Remove the supplied ScinthDefs from the RootNode map.
     *
     * \param names A list of ScinthDef names to remove.
     */
    void defFree(const std::vector<std::string>& names);

    /*! Recursively frees all nodes within each nodeID specified in the list. Note that both Groups and Scinths are
     * freed using this same method.
     *
     * \param nodeIDs The list of nodeIDs to free.
     */
    void nodeFree(const std::vector<int>& nodeIDs);

    /*! Sets the pause/play status of provided nodeID in the provided list of pairs.
     *
     * \param pairs A pair of integers, with the first element as a nodeID and the second as a run value. A value of
     *        zero for the run value will pause the nodeID, and a nonzero value will play it.
     */
    void nodeRun(const std::vector<std::pair<int, int>>& pairs);

    /*! Sets the control parameters for the given node.
     *
     * \param nodeID The id of the node to set. If the node is a group this will set the controls for every node
     *        in the group.
     * \param namedValues A list of pairs of <parameterName, value> to set on the node.
     * \param indexedValues A list of pairs of <parameterIndex, value> to set on the node.
     */
    void nodeSet(int nodeID, const std::vector<std::pair<std::string, float>>& namedValues,
                 const std::vector<std::pair<int, float>>& indexedValues);

    /*! Moves the provided nodes before another.
     *
     * \param nodes The pairs of node IDs <A, B>. In each pair A will be moved to be immediately before B.
     */
    void nodeBefore(const std::vector<std::pair<int, int>>& nodes);

    /*! Moves the provided nodes after another.
     *
     * \param nodes The pairs of node IDs <A, B>. In each pair A will be moved immediately after B.
     */
    void nodeAfter(const std::vector<std::pair<int, int>>& nodes);

    /*! Move and order a list of nodes.
     *
     * \param addAction Where to put the list of nodes relative to targetID. Note that kReplace is not supported.
     * \param targetID The target node relevant to the addAction
     * \param nodeIDs The ordered list of nodeIDs to move.
     */
    void nodeOrder(AddAction addAction, int targetID, const std::vector<int>& nodeIDs);

    /*! Create a new Scinth, cue it to start on next frame, and add it to the tree.
     *
     *  \param scinthDefName The name of the ScinthDef to use in creating the Scinth
     *  \param nodeID The nodeID to assign. If negative, generate a unique negative value automatically.
     *  \param addAction Where to put the new Scinth relative to targetID
     *  \param targetID The relevant node for the addAction
     *  \param namedValues A list of pairs <parameterName, value> to override default values to the Scinth
     *  \param indexedValues A list of pairs <parameterIndex, value> to override values to the Scinth
     *
     */
    void scinthNew(const std::string& scinthDefName, int nodeID, AddAction addAction, int targetID,
                   const std::vector<std::pair<std::string, float>>& namedValues,
                   const std::vector<std::pair<int, float>>& indexedValues);

    /*! Create a new group.
     *
     * \param gruops A tuple <groupID, addAction, targetID>, with ID to assign to the new group (must be unique),
     *        where to put the new group relative to targetID, and the nodeID relative to addAction
     */
    void groupNew(const std::vector<std::tuple<int, AddAction, int>>& groups);

    /*! Move nodes to the head of a group, for execution first in the group.
     *
     * \param nodes A list of pairs of <groupID, nodeID> where each node will be moved to the head of paired group.
     */
    void groupHead(const std::vector<std::pair<int, int>>& nodes);

    /*! Move nodes to the tail of a group, for execution last in the group.
     *
     * \param nodes A list of pairs of <groupID, nodeID> where each node will be moved to the tail of paired group.
     */
    void groupTail(const std::vector<std::pair<int, int>>& nodes);

    /*! Delete all nodes in a group. The group containing the nodes remains. Any sub-groups are freed.
     *
     * \param groups A list of top-level groups to delete nodes from.
     */
    void groupFreeAll(const std::vector<int>& groupIDs);

    /*! Free Scinths only in this group and subgroups.
     *
     * \param groups A list of top-level groups to delete Scinths from.
     */
    void groupDeepFree(const std::vector<int>& groupIDs);

    /*! Post a description of this group's subtree to the log.
     *
     * \param pairs A pair of <groupID, flag>, each groupID will be posted, if flag is nonzero will also post Scinth
     *        control values.
     */
    void groupDumpTree(const std::vector<std::pair<int, int>>& pairs);

    /*! TODO
     */
    void groupQueryTree();

    /*! Prepare to copy the provided decoded image to a suitable GPU-local buffer.
     *
     * \param imageID The integer image identifier for this
     * \param width The width of the image in pixels.
     * \param height The height of the image in pixels.
     * \param imageBuffer The bytes of the image in RGBA format.
     * \param completion A function to call once the image has been staged.
     */
    void stageImage(int imageID, uint32_t width, uint32_t height, std::shared_ptr<vk::HostBuffer> imageBuffer,
                    std::function<void()> completion);

    /*! Returns basic information about the image associated with imageID, if it exists.
     *
     * \param imageID The image ID to query
     * \param sizeOut Will store the size in bytes of the image buffer
     * \param widthOut Will store the width of the image buffer in pixels
     * \param heightOut Will store the height of the image buffer in pixels
     * \return True if the image was found and the output values have been written, false otherwise.
     */
    bool queryImage(int imageID, size_t& sizeOut, uint32_t& widthOut, uint32_t& heightOut);

    /*! Adds an audio Ingress object for provision of audio data to the GPU.
     *
     * \param ingress The ingress object to consume audio from. RootNode will attempt to extract audio data from the
     *        source on every call to prepareFrame().
     * \param imageID The image ID to associate with this audio stream, for sampling.
     * \return true on success, false on failure.
     */
    bool addAudioIngress(std::shared_ptr<audio::Ingress> ingress, int imageID);

    /*! Returns the size of the render tree in nodes.
     */
    size_t numberOfRunningNodes();

    std::shared_ptr<StageManager> stageManager() { return m_stageManager; }

protected:
    void rebuildCommandBuffer(std::shared_ptr<FrameContext> context);

    std::shared_ptr<vk::Device> m_device;
    std::shared_ptr<Canvas> m_canvas;
    std::unique_ptr<ShaderCompiler> m_shaderCompiler;
    std::shared_ptr<vk::CommandPool> m_computeCommandPool;
    std::shared_ptr<vk::CommandPool> m_drawCommandPool;
    std::shared_ptr<StageManager> m_stageManager;
    std::shared_ptr<SamplerFactory> m_samplerFactory;
    std::shared_ptr<ImageMap> m_imageMap;
    bool m_commandBuffersDirty;
    std::atomic<int> m_nodeSerial;

    std::mutex m_scinthDefMutex;
    std::unordered_map<std::string, std::shared_ptr<ScinthDef>> m_scinthDefs;

    std::shared_ptr<vk::CommandBuffer> m_computePrimary;
    std::shared_ptr<vk::CommandBuffer> m_drawPrimary;

    // Protects m_scinths, m_scinthMap, m_groupMap and m_audioStagers
    std::mutex m_treeMutex;
    struct Group {
        // Range within list m_scinths
        std::list<std::shared_ptr<Scinth>>::iterator scinthBegin;
        std::list<std::shared_ptr<Scinth>>::iterator scinthEnd;
        // Range within list m_groups
        std::list<Group>::iterator groupBegin;
        std::list<Group>::iterator groupEnd;
        int groupID;
    };

    std::list<std::shared_ptr<Scinth>> m_scinths;
    // Flat map of every node in the tree, for (amortized) O(1) access to individual nodes by ID and maintaining
    // guarantee that each nodeID uniquely identifies a single node in the tree. The value is an iterator from the child
    // list of the parent Node containing this one.
    std::unordered_map<int, std::list<std::shared_ptr<Scinth>>::iterator> m_scinthMap;

    std::list<Group> m_groups;
    std::unordered_map<int, std::list<Group>::iterator> m_groupMap;

    std::vector<std::shared_ptr<AudioStager>> m_audioStagers;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_ROOT_NODE_HPP_
