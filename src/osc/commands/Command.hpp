#ifndef SRC_OSC_COMMANDS_COMMAND_HPP_
#define SRC_OSC_COMMANDS_COMMAND_HPP_

#include "lo/lo.h"

#include <string>

namespace scin { namespace osc {

class Dispatcher;

namespace commands {

class Command {
public:
    enum Number : int {
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
        kDRecv = 5,
        kDLoad = 6,
        kDLoadDir = 7,
        kDFree = 8,

        // Node Commands
        kNFree = 11,
        kNRun = 12,

        // Scinth Commands
        kSNew = 9,

        // Non Real Time Commands
        kNRTScreenShot = 67,
        kNRTAdvanceFrame = 68,

        // Not a command, an upper bound for Command counts.
        kCommandCount = 70
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
    virtual void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) = 0;

protected:
    Dispatcher* m_dispatcher;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_COMMAND_HPP_
