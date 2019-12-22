#ifndef SRC_SCINTHDEF_HPP_
#define SRC_SCINTHDEF_HPP_

#include <vector>

namespace scin {

class VGen;

/*! Maintains a topologically sorted signal graph of VGen objects and can produce Scinth instances.
 */
class ScinthDef {
public:
    /*! Copy the supplied list of VGens into self and construct a ScinthDef.
     */
    ScinthDef(const std::vector<VGen>& instances);

    /*! Destructs a ScinthDef and all associated resources.
     */
    ~ScinthDef();

    bool buildShaders(bool keepSources);

    const VGen& instanceAt(int index) const { return m_instances[index]; }
    size_t numberOfInstances() const { return m_instances.size(); }

private:
    std::vector<VGen> m_instances;
};

} // namespace scin

#endif // SRC_SCINTHDEF_HPP_
