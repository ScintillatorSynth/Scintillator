#ifndef SRC_CORE_SCINTHDEF_PARSER_HPP_
#define SRC_CORE_SCINTHDEF_PARSER_HPP_

#include "base/AbstractSampler.hpp"
#include "base/VGen.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace YAML {
class Node;
}

namespace scin { namespace base {

class AbstractScinthDef;
class AbstractVGen;

/*! Parses input yaml to produce AbstractScinthDef and AbstractVGen objects.
 */
class Archetypes {
public:
    Archetypes();
    ~Archetypes();

    /* Parse ScinthDefs from the supplied yaml file.
     *
     * \param fileName A path and filename of the file to load and parse.
     * \return The number of valid ScinthDefs parsed, or a negative number on unrecoverable error.
     */
    std::vector<std::shared_ptr<const AbstractScinthDef>> loadFromFile(const std::string& fileName);

    /*! Parse ScinthDefs from a yaml string.
     *
     * \param yaml The string containing one or more ScinthDef yaml documents to parse and load.
     * \return The number of valid ScinthDefs parsed, or a negative number on unrecoverable error.
     */
    std::vector<std::shared_ptr<const AbstractScinthDef>> parseFromString(const std::string& yaml);

    /*! Lookup a ScinthDef by name and return if found.
     *
     * \param name The unique name of the ScinthDef to find.
     * \return A pointer to the ScinthDef, or nullptr if not found.
     */
    std::shared_ptr<const AbstractScinthDef> getAbstractScinthDefNamed(const std::string& name);

    /*! Remove the AbstractScinthDefs from the map as identified by name.
     *
     * \param names A list of names of AbstractScinthDefs to remove.
     */
    void freeAbstractScinthDefs(const std::vector<std::string>& names);

    /*! The size of the current ScinthDefs dictionary.
     *
     * \return The number of defined ScinthDefs.
     */
    size_t numberOfAbstractScinthDefs();

    /*! Parse AbstractVGens from the supplied yaml file, and add them to the internal dictionary.
     *
     * \param fileName A path and filename of the file to load and parse.
     * \return The number of valid AbstractVGens parsed, or a negative number on load error.
     */
    int loadAbstractVGensFromFile(const std::string& fileName);

    /*! Parse AbstractVGens from a yaml string.
     *
     * \param yaml The string containing one or more AbstractVGen yaml documents to parse and load.
     * \return The number of valid AbstractVGens parsed, or a negative number on parse error.
     */
    int parseAbstractVGensFromString(const std::string& yaml);

    /*! Lookup an AbstractVGen by name and return if found.
     *
     * \param name The unique name of the AbstractVGen to find.
     * \return A pointer to the AbstractVGen object found, or nullptr if no object found under that name.
     */
    std::shared_ptr<const AbstractVGen> getAbstractVGenNamed(const std::string& name);

    /*! The size of the current AbstractVGen dictionary.
     *
     * \return The number of unique AbstractVGens currently loaded and validated by the system.
     */
    size_t numberOfAbstractVGens();

    /*! Parses but does not validate, build, or add to the ScinthDef map. Used for testing.
     *
     * \param yaml The yaml string assumed to contain a single ScinthDef
     * \return A pointer to the parsed AbstractScinthDef, or nullptr on parsing error.
     */
    std::shared_ptr<AbstractScinthDef> parseOnly(const std::string& yaml);

private:
    std::vector<YAML::Node> parseYAMLFile(const std::string& fileName);
    std::vector<YAML::Node> parseYAMLString(const std::string& yaml);

    std::vector<std::shared_ptr<const AbstractScinthDef>> extractFromNodes(const std::vector<YAML::Node>& nodes);
    std::shared_ptr<AbstractScinthDef> extractSingleNode(const YAML::Node& node);

    /*! Builds individual AbstractVGen objects from the parsed yaml data structures and stores them in the dictionary.
     *
     * \param nodes The vector of nodes extracted from the file or string.
     * \return The number of valid VGens extracted.
     */
    int extractAbstractVGensFromNodes(const std::vector<YAML::Node>& nodes);

    std::mutex m_vgensMutex;
    std::unordered_map<std::string, std::shared_ptr<const AbstractVGen>> m_abstractVGens;

    std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<const AbstractScinthDef>> m_scinthDefs;

    std::unordered_map<std::string, VGen::InputType> m_vgenInputTypes;
    std::unordered_map<std::string, AbstractSampler::FilterMode> m_samplerFilterModes;
    std::unordered_map<std::string, AbstractSampler::AddressMode> m_samplerAddressModes;
    std::unordered_map<std::string, AbstractSampler::ClampBorderColor> m_samplerBorderColors;
};

} // namespace base

} // namespace scin

#endif // SRC_CORE_SCINTHDEF_PARSER_HPP_
