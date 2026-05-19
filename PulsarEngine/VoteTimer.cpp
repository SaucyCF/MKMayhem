#include <kamek.hpp>
#include <MarioKartWii/UI/Ctrl/CountDown.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <PulsarSystem.hpp>

namespace Pulsar {

extern "C" void CountDownHAW(CountDown* self, float) {
    float countdown = 30.0f;
    u32 seconds = 30;

    if (System::sInstance->IsContext(Pulsar::PULSAR_HAW)) {
        const RKNet::Controller* controller = RKNet::Controller::sInstance;
        if (controller != nullptr) {
            const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];
            const bool isNonHost = sub.localAid != sub.hostAid;
            if (isNonHost) {
                countdown = 10.0f;
                seconds = 10;
            }
        }
    }
    self->countdown = countdown;
    self->seconds = seconds;
    self->isActive = false;
}
kmBranch(0x805C3C2C, CountDownHAW);

}  // namespace Pulsar