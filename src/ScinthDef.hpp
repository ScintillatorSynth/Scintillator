#ifndef SRC_SCINTHDEF_HPP_
#define SRC_SCINTHDEF_HPP_

#include <vector>

namespace scin {

class VGenInstance;

/*! Maintains a signal graph of VGen objects and can produce Scinth instances.
 */
class ScinthDef {
public:
    ScinthDef(const std::vector<VGenInstance>& instances);
    ~ScinthDef();

private:
    std::vector<VGenInstance> m_instances;
};

} // namespace scin

#endif // SRC_SCINTHDEF_HPP_
