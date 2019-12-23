#ifndef SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
#define SRC_CORE_ABSTRACT_SCINTHDEF_HPP_

#include <vector>

namespace scin {

class VGen;

/*! Maintains a topologically sorted signal graph of VGens and constructs shaders and requirements for the graphical
 * ScinthDef instances.
 */
class AbstractScinthDef {
public:
    /*! Copy the supplied list of VGens into self and construct an AbstractScinthDef.
     */
    AbstractScinthDef(const std::vector<VGen>& instances);

    /*! Destructs an AbstractScinthDef and all associated resources.
     */
    ~AbstractScinthDef();

    bool buildShaders(bool keepSources);

    const VGen& instanceAt(int index) const { return m_instances[index]; }
    size_t numberOfInstances() const { return m_instances.size(); }

private:
    std::vector<VGen> m_instances;
};

} // namespace scin

#endif // SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
