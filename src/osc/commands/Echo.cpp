#include "osc/commands/Echo.hpp"

#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace osc { namespace commands {

Echo::Echo(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Echo::~Echo() {}

void Echo::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc < 1 || types[0] != LO_BLOB) {
        spdlog::error("OSC Echo missing/incorrect blob message argument.");
        return;
    }

    lo_blob blob = *reinterpret_cast<lo_blob*>(argv[0]);
    uint32_t echoMessageSize = std::min(lo_blob_datasize(blob), static_cast<uint32_t>(LO_MAX_MSG_SIZE));
    if (echoMessageSize > 0) {
        std::shared_ptr<uint8_t[]> echoMessage(new uint8_t[echoMessageSize]);
        std::memcpy(echoMessage.get(), lo_blob_dataptr(blob), echoMessageSize);
    }
}

} // namespace commands
} // namespace osc
} // namespace scin
