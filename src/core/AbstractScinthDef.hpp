#ifndef SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
#define SRC_CORE_ABSTRACT_SCINTHDEF_HPP_

#include "Intrinsic.hpp"
#include "Manifest.hpp"

#include <unordered_set>
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
     * \param vgenIndex The index of the VGen in this AbstractScinthDef.
     * \param outputIndex The index of the VGen output to reference.
     * \return The standardized name of the VGen, or empty string on error.
     */
    std::string nameForVGenOutput(int vgenIndex, int outputIndex) const;

    const std::string& name() const { return m_name; }
    const VGen& instanceAt(int index) const { return m_instances[index]; }
    size_t numberOfInstances() const { return m_instances.size(); }
    const std::unordered_set<Intrinsic> intrinsics() const { return m_intrinsics; }
    const std::string& uniquePrefix() const { return m_uniquePrefix; }
    const std::string& vertexShader() const { return m_vertexShader; }
    const std::string& fragmentShader() const { return m_fragmentShader; }
    const Manifest& vertexManifest() const { return m_vertexManifest; }
    const Manifest& vertexUniformManifest() const { return m_vertexUniformManifest; }
    const Manifest& fragmentUniformManifest() const { return m_fragmentUniformManifest; }

private:
    bool buildParameters();
    bool buildShaders();

    std::string m_name;
    std::vector<VGen> m_instances;

    std::string m_uniquePrefix;
    std::unordered_set<Intrinsic> m_intrinsics;
    std::vector<std::vector<std::string>> m_inputs;
    std::vector<std::vector<std::string>> m_outputs;
    std::string m_vertexShader;
    std::string m_fragmentShader;
    Manifest m_vertexManifest;
    Manifest m_vertexUniformManifest;
    Manifest m_fragmentUniformManifest;
};

} // namespace scin

#endif // SRC_CORE_ABSTRACT_SCINTHDEF_HPP_