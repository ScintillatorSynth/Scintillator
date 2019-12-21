#ifndef SRC_SCINTHDEF_MANAGER_HPP_
#define SRC_SCINTHDEF_MANAGER_HPP_

#include <memory>
#include <unordered_map>
#include <vector>

namespace YAML {
class Node;
}

namespace scin {

class ScinthDef;
class VGenManager;

/*! Maintains the list of currently defined ScinthDefs. Can parse yaml ScinthDef input.
 */
class ScinthDefManager {
public:
    ScinthDefManager(std::shared_ptr<VGenManager> vgenManager);
    ~ScinthDefManager();

    /* Parse ScinthDefs from the supplied yaml file.
     *
     * \param fileName A path and filename of the file to load and parse.
     * \return The number of valid ScinthDefs parsed, or a negative number on unrecoverable error.
     */
    int loadFromFile(const std::string& fileName);

    /*! Parse ScinthDefs from a yaml string.
     *
     * \param yaml The string containing one or more ScinthDef yaml documents to parse and load.
     * \return The number of valid ScinthDefs parsed, or a negative number on unrecoverable error.
     */
    int parseFromString(const std::string& yaml);

private:
    int extractFromNodes(const std::vector<YAML::Node>& nodes);
    bool extractFromNode(const YAML::Node& node);

    std::shared_ptr<VGenManager> m_vgenManager;
    std::unordered_map<std::string, std::shared_ptr<ScinthDef>> m_scinthDefs;
};

} // namespace scin

#endif // SRC_SCINTHDEF_MANAGER_HPP_
