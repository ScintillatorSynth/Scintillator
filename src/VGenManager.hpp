#ifndef SRC_VGEN_MANAGER_HPP_
#define SRC_VGEN_MANAGER_HPP_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace YAML {
class Node;
}

namespace scin {

class AbstractVGen;

/*! Responsible for the parsing, storage, retrieval, and updating of VGen objects.
 *
 * VGenManager is intended to keep the authoritative list of all VGen objects known by the system. It supports
 * multi-threaded access at the cost of a mutex acquisition per access. It supports uniquely identifying VGens by name.
 * Adding a new VGen with the same name as one already registered with VGenManager will result in the old VGen being
 * overwritten.
 */
class VGenManager {
public:
    VGenManager();
    ~VGenManager();

    /*! Parse VGens from the supplied yaml file.
     *
     * \param fileName A path and filename of the file to load and parse.
     * \return The number of valid VGens parsed, or a negative number on load error.
     */
    int loadFromFile(const std::string& fileName);

    /*! Parse VGens from a yaml string.
     *
     * \param yaml The string containing one or more VGen yaml documents to parse and load.
     * \return The number of valid VGens parsed, or a negative number on parse error.
     */
    int parseFromString(const std::string& yaml);

    /*! Lookup a VGen by name and return if found.
     *
     * \param name The unique name of the VGen to find.
     * \return A pointer to the VGen object found, or nullptr if no object found under that name.
     */
    std::shared_ptr<const AbstractVGen> getVGenNamed(const std::string& name);

    /*! The size of the current VGen dictionary.
     *
     * \return The number of unique VGens currently loaded and validated by the system.
     */
    size_t numberOfVGens();

private:
    /*! Pulls individual VGen objects from the parsed yaml data structures and stores them in the dictionary.
     *
     * \param nodes The vector of nodes extracted from the file or string.
     * \return The number of valid VGens extracted.
     */
    int extractFromNodes(const std::vector<YAML::Node>& nodes);

    /*! Pull an single VGen from the parsed yaml data structure and store it in the dictionary.
     *
     * \param node The yaml node that is considered to be a VGen.
     * \return true if node was a valid VGen, false on error.
     */
    bool extractFromNode(YAML::Node& node);

    std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<const AbstractVGen>> m_VGens;
};

} // namespace scin

#endif // SRC_VGEN_MANAGER_HPP_
