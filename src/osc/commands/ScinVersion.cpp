#include "osc/commands/ScinVersion.hpp"

#include "Version.hpp"
#include "osc/Address.hpp"
#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

ScinVersion::ScinVersion(osc::Dispatcher* dispatcher): Command(dispatcher) {}

ScinVersion::~ScinVersion() {}

void ScinVersion::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->respond(origin, "/scin_version.reply", "scinsynth", kScinVersionMajor, kScinVersionMinor,
                          kScinVersionPatch, kScinBranch, kScinCommitHash);
}

} // namespace commands
} // namespace osc
} // namespace scin
