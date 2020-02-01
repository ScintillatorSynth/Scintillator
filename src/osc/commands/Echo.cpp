#include "osc/commands/Echo.hpp"

#include "osc/Address.hpp"
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

    lo_blob blob = reinterpret_cast<lo_blob>(argv[0]);
    uint32_t blobSize = lo_blob_datasize(blob);
    if (blobSize == 0) {
        spdlog::error("OSC Echo got empty blob message argument.");
        return;
    }

    void* blobData = lo_blob_dataptr(blob);
    lo_message message = lo_message_deserialise(blobData, blobSize, nullptr);
    if (!message) {
        spdlog::error("OSC Echo failed to deserialize blob message argument.");
        return;
    }

    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->respond(origin, reinterpret_cast<const char*>(blobData), message);
}

} // namespace commands
} // namespace osc
} // namespace scin
