#include "infra/CrashReporter.hpp"

#include <client/crashpad_client.h>

namespace scin { namespace infra {

CrashReporter::CrashReporter() {
    m_client = crashpad::CrashpadClient();
}

CrashReporter::~CrashReporter() {
}

bool CrashReporter::startCrashHandler() {

}

} // namespace infra
} // namespace scin

