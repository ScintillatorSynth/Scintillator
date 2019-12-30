#ifndef SRC_CORE_MANIFEST_HPP_
#define SRC_CORE_MANIFEST_HPP_

#include <string>
#include <unordered_map>
#include <vector>

namespace scin {

/*! Describes a configurable buffer layout that aligns members consistent with Vulkan standards.
 *
 *  The Vulkan standard is that an element of size n bytes needs to be aligned on a n-byte boundary. The Manifest allows
 *  for adding elements of unique name and selectable type, in any order. In order to ensure an efficient use of memory,
 *  Manifest follows a greedy strategy and assigns elements generally in descending order of size, but attempts to fit
 *  in smaller elements into the alignment padding. After calling pack() the Manifest provides access to information
 *  about the name, offset, and size of each element included.
 */
class Manifest {
public:
    /*! Construct an empty Manifest.
     */
    Manifest();

    /*! Destruct a Manifest.
     */
    ~Manifest();

    enum ElementType { kFloat, kVec2, kVec3, kVec4 };

    /*! Add an element to Manifest, if name is unique.
     *
     * \param name The name to associate this element with.
     * \param type The type of element to add, which will also control the size of storage reserved.
     * \return True if successfully added, false if not (name must be unique).
     */
    bool addElement(const std::string& name, ElementType type);

    /*! Create the final description of the data structure. Call after adding all elements but before accessing any
     * of the informational functions about layout.
     */
    void pack();

    // Things we need to know:
    size_t numberOfElements() const { return 0; }
    uint32_t sizeInBytes() const { return 0; }
    // Returns offset in units of bytes.
    uint32_t offsetForElement(const std::string& name) const { return 0; }
    const std::string& nameForElement(size_t index) const { return ""; }

private:
    size_t m_numberOfElements;
    size_t m_size;
};

} // namespace scin

#endif // SRC_CORE_MANIFEST_HPP_
