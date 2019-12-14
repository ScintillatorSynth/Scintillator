#ifndef SRC_VGEN_HPP_
#define SRC_VGEN_HPP_

#include <string>

namespace scin {

class VGen {
public:
    VGen();
    ~VGen();

private:
    std::string m_name;
    std::string m_code;
};

}  // namespace scin

#endif  // SRC_VGEN_HPP_

