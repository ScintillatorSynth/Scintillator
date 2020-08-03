#ifndef SRC_COMP_NODE_HPP_
#define SRC_COMP_NODE_HPP_

namespace scin { namespace comp {

/*! Abstract base class for the individual elements within a rendering tree. Descendants are RootNode, Group, and
 *  Scinth.
 */
class Node {
public:
    Node(int nodeID);
    virtual ~Node();

    virtual bool create() = 0;
    virtual void destroy() = 0;
    virtual bool prepareFrame(size_t imageIndex, double frameTime) = 0;

    /*! Determines the paused or playing status of the Node. TODO: should paused nodes still render? Unlike in audio,
     * a paused VGen can still produce a still frame.
     *
     * \param run If false, will pause the Node. If true, will play it.
     */
    void setRunning(bool run) { m_running = run; }

protected:
    int m_nodeID;
    bool m_running;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_NODE_HPP_
