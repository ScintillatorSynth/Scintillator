#ifndef SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
#define SRC_CORE_ABSTRACT_SCINTHDEF_HPP_

#include <set>
#include <string>
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
    AbstractScinthDef(const std::string& name, const std::vector<VGen>& instances);

    /*! Destructs an AbstractScinthDef and all associated resources.
     */
    ~AbstractScinthDef();

    /*! Construct shaders, uniform list, and other requirements for rendering this ScinthDef.
     *
     * \return true if successful, false on error.
     */
    bool build();

    /*! Returns the standardized name for the output of the VGen at the supplied index.
     *
     * \note An internal method, exposed mostly for testing.
     * \param index The index of the VGen in this AbstractScinthDef.
     * \return The standardized name of the VGen, or empty string on error.
     */
    std::string nameForVGenOutput(int index) const;

    const VGen& instanceAt(int index) const { return m_instances[index]; }
    size_t numberOfInstances() const { return m_instances.size(); }

private:
    std::string m_name;
    std::vector<VGen> m_instances;

    std::string m_uniquePrefix;
    std::set<std::string> m_uniforms;
    std::vector<std::vector<std::string>> m_inputs;
    std::vector<std::vector<std::string>> m_intrinsics;
    std::vector<std::vector<std::string>> m_intermediates;
    std::string m_vertexShader;
    std::string m_fragmentShader;
};

} // namespace scin

#endif // SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
