#include "av/Decoder.hpp"

namespace scin { namespace av {

Decoder::Decoder(): m_width(0), m_height(0), m_codecContext(nullptr), m_formatContext(nullptr) {}

Decoder::~Decoder() {}

} // namespace av
} // namespace scin
