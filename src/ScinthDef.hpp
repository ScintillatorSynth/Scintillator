#ifndef SRC_SCINTHDEF_HPP_
#define SRC_SCINTHDEF_HPP_

#include <memory>

namespace scin {

class AbstractScinthDef;

/*! A ScinthDef encapsulates all of the graphics state that can be reused across individual instances of Scinths.
 */
class ScinthDef {
public:
    ScinthDef(std::shared_ptr<const AbstractScinthDef> abstractScinthDef);
    ~ScinthDef();

private:
};

} // namespace scin

#endif // SRC_SCINTHDEF_HPP_
