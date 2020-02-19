#ifndef SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
#define SRC_CORE_ABSTRACT_SCINTHDEF_HPP_

#include "core/Intrinsic.hpp"
#include "core/Manifest.hpp"
#include "core/Parameter.hpp"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <memory>

namespace scin { namespace core {

class Shape;
class VGen;

/*! Maintains a topologically sorted signal graph of VGens and constructs shaders and requirements for the graphical
 * ScinthDef instances.
 */
class AbstractScinthDef {
public:
    /*! Copy the supplied list of VGens into self and construct an AbstractScinthDef.
     *
     * \param name The name of this ScinthDef.
     * \param parameters A list of parameter objects, in order.
     * \param instances The VGens in topological order from inputs to output.
     */
    AbstractScinthDef(const std::string& name, const std::vector<Parameter>& parameters,
                      const std::vector<VGen>& instances);

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

    /*! Returns the index for a given parameter name, or -1 if name not found.
     *
     * \name The name of the parameter to look up.
     * \return The index of the parameter with the supplied name or -1 if not found.
     */
    int indexForParameterName(const std::string& name) const;

    const std::string& name() const { return m_name; }
    const std::vector<Parameter>& parameters() const { return m_parameters; }
    const std::vector<VGen>& instances() const { return m_instances; }
    const Shape* shape() const { return m_shape.get(); }
    const std::unordered_set<Intrinsic> intrinsics() const { return m_intrinsics; }
    const std::string& prefix() const { return m_prefix; }
    const std::string& vertexShader() const { return m_vertexShader; }
    const std::string& fragmentShader() const { return m_fragmentShader; }
    const Manifest& vertexManifest() const { return m_vertexManifest; }
    const Manifest& uniformManifest() const { return m_uniformManifest; }
    const std::string& vertexPositionElementName() const { return m_vertexPositionElementName; }
    const std::string& parametersStructName() const { return m_parametersStructName; }

private:
    bool buildInputs();
    bool buildNames();
    bool buildManifests();
    bool buildVertexShader();
    bool buildFragmentShader();

    std::string m_name;
    std::vector<Parameter> m_parameters;
    std::vector<VGen> m_instances;
    std::unique_ptr<Shape> m_shape;

    std::unordered_set<int> m_fixedImageIDs;
    std::unordered_set<int> m_fixedSamplerIDs;
    std::unordered_set<int> m_imageParameterIndices;
    std::unordered_set<int> m_samplerParameterIndices;

    std::string m_prefix;
    std::string m_vertexPositionElementName;
    std::string m_fragmentOutputName;
    std::string m_parametersStructName;
    std::unordered_map<std::string, int> m_parameterIndices;
    std::unordered_set<Intrinsic> m_intrinsics;
    std::vector<std::vector<std::string>> m_inputs;
    std::vector<std::vector<std::string>> m_outputs;
    std::vector<std::vector<int>> m_outputDimensions;
    std::string m_vertexShader;
    std::string m_fragmentShader;
    Manifest m_vertexManifest;
    Manifest m_uniformManifest;
};

} // namespace core

} // namespace scin

#endif // SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
