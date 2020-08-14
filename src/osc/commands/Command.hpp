#ifndef SRC_OSC_COMMANDS_COMMAND_HPP_
#define SRC_OSC_COMMANDS_COMMAND_HPP_

#include "osc/LOIncludes.hpp"

#include <string>

namespace scin { namespace osc {

class Dispatcher;

namespace commands {

class Command {
public:
    // Where it makes sense, trying to keep these numbers the same as the corresponding SuperCollider server values.
    // Receiving a numeric argument as the command path is not currently supported but should be relatively
    // straightforward to add, assuming liblo supports it.
    enum Number : size_t {
        // Master Controls
        kNone = 0,
        kNotify = 1,
        kStatus = 2,
        kQuit = 3,
        kDumpOSC = 39,
        kSync = 52,
        kLogLevel = 58,
        kVersion = 64,

        // Scinth Definition Commands
        kDefRecv = 5,
        kDefLoad = 6,
        kDefLoadDir = 7,
        kDefFree = 8,

        // Node Commands
        kNodeFree = 11,
        kNodeRun = 12,
        kNodeSet = 15,
        kNodeBefore = 18,
        kNodeAfter = 19,
        kNodeOrder = 62,

        // Scinth Commands
        kScinthNew = 9,

        // Group Commands
        kGroupNew = 21,
        kGroupHead = 22,
        kGroupTail = 23,
        kGroupFreeAll = 24,
        kGroupDeepFree = 50,
        kGroupDumpTree = 56,
        kGroupQueryTree = 57,

        // ImageBuffer Commands
        kImageBufferAllocRead = 29,
        kImageBufferQuery = 47,

        // Non Real Time Commands
        kScreenShot = 67,
        kAdvanceFrame = 68,

        // Testing Utility Commands
        kEcho = 69,
        kLogAppend = 70,
        kSleepFor = 71,
        kLogCrashReports = 73,
        kUploadCrashReport = 74,

        // Not a command, an upper bound for Command counts.
        kCommandCount = 75
    };

    /*! Returns Number enum associated with name, or kNone if no Command found by that name.
     *
     * \param name The name of the command, without the "scin_" prefix.
     * \param length The length of the name string.
     * \return The Number enum associated with name, or kNone if not found.
     */
    static Number getNumberNamed(const char* name, size_t length);

    Command(Dispatcher* dispatcher);
    virtual ~Command();

    /*! Override for subclasses to process incoming commands with arguments.
     *
     * \param argc Number of arguments.
     * \param argv A table of lo_arg data structures with the unpacked argument values.
     * \param types A string of length argc with type codes for each element.
     */
    virtual void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) = 0;

protected:
    Dispatcher* m_dispatcher;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_COMMAND_HPP_
