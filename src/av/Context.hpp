#ifndef SRC_AV_CONTEXT_HPP_
#define SRC_AV_CONTEXT_HPP_

#include "av/AVIncludes.hpp"

namespace scin { namespace av {

// TODO: deprecate.
/*! Abstract base class for media encoding and decoding.
 */
class Context {
public:
    Context();
    virtual ~Context();

protected:
    bool createCodecContext(AVCodecID codecID);

    AVCodecContext* m_context;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_CONTEXT_HPP_
