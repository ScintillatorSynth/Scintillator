#ifndef SRC_CORE_PARAMETER_HPP_
#define SRC_CORE_PARAMETER_HPP_

#include <string>

namespace scin { namespace core {

/*! Simple storage class representing a named numeric value changeable during a Scinth runtime.
 */
class Parameter {
public:
    Parameter(std::string name, float defaultValue);
    ~Parameter();

    const std::string& name() const { return m_name; }
    float defaultValue() const { return m_defaultValue; }

private:
    std::string m_name;
    float m_defaultValue;
};

} // namespace core
} // namespace scin

#endif // SRC_CORE_PARAMETER_HPP_
