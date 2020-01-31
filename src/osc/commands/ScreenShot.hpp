#ifndef SRC_OSC_COMMANDS_SCREEN_SHOT_HPP_
#define SRC_OSC_COMMANDS_SCREEN_SHOT_HPP_

#include "osc/commands/Command.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace scin {

namespace av {
class ImageEncoder;
}

namespace osc { namespace commands {

class ScreenShot : public Command {
public:
    ScreenShot(osc::Dispatcher* dispatcher);
    virtual ~ScreenShot();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;

private:
    std::mutex m_mutex;
    size_t m_encodersSerial;
    std::unordered_map<int, std::shared_ptr<av::ImageEncoder>> m_encoders;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_SCREEN_SHOT_HPP_
