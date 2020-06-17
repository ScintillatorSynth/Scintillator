#ifndef SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
#define SRC_CORE_ABSTRACT_SCINTHDEF_HPP_

#include "base/AbstractVGen.hpp"
#include "base/Intrinsic.hpp"
#include "base/Manifest.hpp"
#include "base/Parameter.hpp"

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <vector>
#include <memory>

namespace scin { namespace base {

class Shape;
class VGen;

/*! Maintains a topologically sorted signal graph of VGens and constructs shaders and requirements for the graphical
 * ScinthDef instances.
 *
 * The AbstractScinthDef takes as input the VGen graph with named parameters, and produces a plain data (meaning no
 * Vulkan-specific data structures) representation of a series of data structures and render commands needed to
 * configure the Vulkan graphics pipeline to draw a Scinth instance of the declared ScinthDef. Specifically it
 * produces as output:
 *
 * a) An optional compute shader, if there are any frame-rate VGens in the graph.
 * b) An input uniform buffer manifest for the compute shader if it exists
 * c) A uniform buffer manifest shared by the vertex and fragment programs
 * d) A vertex shader input manifest
 * e) A vertex shader containing both any shape-rate VGens as well as necessary pass-through data to the fragment shader
 * f) A vertex shader output manifest, one of the sources of input to the fragment shader
 * g) Lists of unified Samplers both constant and parameterized
 * h) Lists of parameters provided as push constants to all shaders
 * i) A fragment shader with the pixel-rate VGens
 *
 * Overall approach is to take a first pass through the graph from output back to inputs, validating the rate flow and
 * bucketing the VGens into one of compute, vertex, or fragment shader groups. Then we take a forward pass through each
 * group of VGens to generate manifests and shaders.
 *
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
    // First element in pair is sampler key, second element is the imageID.
    const std::set<std::pair<uint32_t, int>>& fixedImages() const { return m_fixedImages; }
    // First element in pair is sampler key, second is parameter index.
    const std::set<std::pair<uint32_t, int>>& parameterizedImages() const { return m_parameterizedImages; }
    const std::string& prefix() const { return m_prefix; }
    const std::string& vertexPositionElementName() const { return m_vertexPositionElementName; }
    const std::string& parametersStructName() const { return m_parametersStructName; }
    const std::unordered_set<Intrinsic> intrinsics() const { return m_intrinsics; }
    const std::string& vertexShader() const { return m_vertexShader; }
    const std::string& fragmentShader() const { return m_fragmentShader; }
    const Manifest& vertexManifest() const { return m_vertexManifest; }
    const Manifest& uniformManifest() const { return m_uniformManifest; }

    /*! Largely private methods for AbstractScinthDef construction, made public for unit test accessibility.
     *
     */

    /*! Recursively traverses the VGens list from output back to inputs, grouping them into compute, vertex, and
     *  fragment shaders. Also checks the progression of rates as valid.
     *
     * \param index The index in m_instances of the node to check in this recursive call. Starts at the end of the
     *        vector and descends to zero.
     * \param maxRate The maximum rate to accept, to validate against rate decreases only from output to input.
     * \param computeVGens A reference to a set in which to store the indices of any frame-rate VGens in the graph.
     * \param vertexVGens A reference to a set in which to store the indices of any shape-rate VGens in the graph.
     * \param fragmentVGens A reference to a set in which to store the indices of any fragment-rate VGens in the
     *        graph.
     * \return true if successful, false on error.
     */
    bool groupVGens(int index, AbstractVGen::Rates rate, std::unordered_set<int>& computeVGens,
            std::unordered_set<int>& vertexVGens, std::unordered_set<int>& fragmentVGens);

private:
    bool buildInputs();
    bool buildNames();
    bool buildManifests();
    bool buildComputeShader();
    bool buildVertexShader();
    bool buildFragmentShader();

    std::string m_name;
    std::vector<Parameter> m_parameters;
    std::vector<VGen> m_instances;
    std::unique_ptr<Shape> m_shape;

    std::set<std::pair<uint32_t, int>> m_fixedImages;
    std::set<std::pair<uint32_t, int>> m_parameterizedImages;

    std::string m_prefix;
    std::string m_vertexPositionElementName;
    std::string m_fragmentOutputName;
    std::string m_parametersStructName;
    std::unordered_map<std::string, int> m_parameterIndices;
    std::unordered_set<Intrinsic> m_intrinsics;
    std::vector<std::vector<std::string>> m_inputs;
    std::vector<std::vector<std::string>> m_outputs;
    std::vector<std::vector<int>> m_outputDimensions;
    std::string m_computeShader;
    std::string m_vertexShader;
    std::string m_fragmentShader;
    Manifest m_computeManifest;
    Manifest m_vertexManifest;
    Manifest m_uniformManifest;
};

} // namespace base

} // namespace scin

#endif // SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
