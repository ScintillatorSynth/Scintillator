#ifndef SRC_AV_AV_INCLUDES_HPP_
#define SRC_AV_AV_INCLUDES_HPP_

#if _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4244)
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#if _MSC_VER
#    pragma warning(pop)
#endif

#endif // SRC_AV_INCLUDES_HPP_
