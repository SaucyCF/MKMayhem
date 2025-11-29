#include <kamek.hpp>
#include <runtimeWrite.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <MarioKartWii/Kart/KartLink.hpp>
#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/Input/ControllerHolder.hpp>
#include <MarioKartWii/Input/InputState.hpp>
#include <PulsarSystem.hpp>

namespace Pulsar {
using namespace Kart;
using namespace Input;

// Code based of vabold and Gaberboo's MKDS MT charging code https://mariokartwii.com/showthread.php?tid=2024&highlight=MKDS
static void InitDriftFieldsForMovement(Movement& mov) {
    mov.unknown_0x254[0] = 0;
    mov.unknown_0x254[1] = 0;
    mov.unknown_0xca[0] = 0;
}

static int UpdateMTCharge_Patched(Movement& movement) {
    if (movement.driftState >= 3) return 0;
    if (!movement.IsLocal()) return 0;

    const float& STICK_CHARGE_THRESHOLD = *(const float*)0x808b5ccc;
    ControllerHolder& ctrl = movement.GetControllerHolder();
    float stickX = ctrl.inputStates[0].stick.x;
    int hopStickX = movement.hopStickX;
    const float negThreshold = -STICK_CHARGE_THRESHOLD;

    if (hopStickX == -1) {
        if (stickX < negThreshold) {
            goto calcDriftState; 
        }

        if (stickX > STICK_CHARGE_THRESHOLD) {
            movement.unknown_0xca[0] = 1;
            return 0;
        }

        return 0;
    }
    if (stickX > STICK_CHARGE_THRESHOLD) {
        goto calcDriftState;
    }
    if (stickX < negThreshold) {
        movement.unknown_0xca[0] = 1;
        return 0;
    }
    return 0;

calcDriftState:
    if (movement.unknown_0xca[0] == 0) return 0;
    movement.driftState = movement.driftState + 1;
    movement.unknown_0xca[0] = 0;
    return 0;
}

kmRuntimeUse(0x80578088);
kmRuntimeUse(0x8057ee50);
static void MKDSDrifting() {
kmRuntimeWrite32A(0x80578088, 0xB3DF0254);
kmRuntimeWrite32A(0x8057ee50, 0xA8A300FC);
if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL) {
      if (System::sInstance->IsContext(Pulsar::PULSAR_MAYHEM)) {
          kmRuntimeCallA(0x80578088, InitDriftFieldsForMovement);
          kmRuntimeBranchA(0x8057ee50, UpdateMTCharge_Patched);
        }
    }
}
PageLoadHook MKDSDriftingPatch(MKDSDrifting);

} //namespace Pulsar
