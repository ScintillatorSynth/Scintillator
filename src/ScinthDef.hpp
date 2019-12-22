#ifndef SRC_SCINTHDEF_HPP_
#define SRC_SCINTHDEF_HPP_

#include <vector>

namespace scin {

class VGenInstance;

/*! Maintains a topologically sorted signal graph of VGenInstance objecs and can produce Scinth instances.
 */
class ScinthDef {
public:
    /*! Copy the supplied list of VGenInstances into self and construct a ScinthDef.
     */
    ScinthDef(const std::vector<VGenInstance>& instances);

    /*! Destructs a ScinthDef and all associated resources.
     */
    ~ScinthDef();

    bool buildShaders(bool keepSources);

    const VGenInstance& instanceAt(int index) const { return m_instances[index]; }
    size_t numberOfInstances() const { return m_instances.size(); }

private:
    std::vector<VGenInstance> m_instances;
};

} // namespace scin

#endif // SRC_SCINTHDEF_HPP_
