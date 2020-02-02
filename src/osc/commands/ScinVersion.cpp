#include "osc/commands/ScinVersion.hpp"

#include "Version.hpp"
#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

ScinVersion::ScinVersion(osc::Dispatcher* dispatcher): Command(dispatcher) {}

ScinVersion::~ScinVersion() {}

void ScinVersion::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    m_dispatcher->respond(address, "/scin_version.reply", "scinsynth", kScinVersionMajor, kScinVersionMinor,
                          kScinVersionPatch, kScinBranch, kScinCommitHash);
}

} // namespace commands
} // namespace osc
} // namespace scin
