#include <kamek.hpp>
#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/Effect/EffectMgr.hpp> 
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <Race/200ccParams.hpp>
#include <PulsarSystem.hpp>
#include <DKW.hpp>

//Unoptimized code which is mostly a port of Stebler's version which itself comes from CTGP's, speed factor is in the LapSpeedModifier code


namespace Pulsar {
namespace Race {

static void CannonExitSpeed() {
    const float ratio = System::sInstance->IsContext(PULSAR_200) || DKW::System::Is400cc() || DKW::System::Is99999cc() ? cannonExit : 1.0f;
    register Kart::Movement* kartMovement;
    asm(mr kartMovement, r30;);
    kartMovement->engineSpeed = kartMovement->baseSpeed * ratio;
}
kmCall(0x805850c8, CannonExitSpeed);

void EnableBrakeDrifting(Input::ControllerHolder& controllerHolder) {
    bool NoBrakeDrift = Pulsar::DKWSETTING_150_BRAKEDRIFT_ON;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    const GameMode gameMode = Racedata::sInstance->menusScenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
       NoBrakeDrift = System::sInstance->IsContext(Pulsar::PULSAR_BDRIFTING) ? Pulsar::DKWSETTING_150_BRAKEDRIFT_OFF : Pulsar::DKWSETTING_150_BRAKEDRIFT_ON;
}

    if(NoBrakeDrift == Pulsar::DKWSETTING_150_BRAKEDRIFT_OFF || System::sInstance->IsContext(PULSAR_MODE_OTT) && !System::sInstance->IsContext(PULSAR_200) || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is400cc() || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is99999cc() || mode == MODE_TIME_TRIAL && !System::sInstance->IsContext(PULSAR_200) || mode == MODE_GHOST_RACE && !System::sInstance->IsContext(PULSAR_200)){}
    else if(static_cast<Pulsar::DKWSettingBrakeDrift>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW1), Pulsar::SETTINGDKW_BRAKEDRIFT)) == Pulsar::DKWSETTING_BRAKEDRIFT_ENABLED || System::sInstance->IsContext(PULSAR_200) || DKW::System::Is400cc() || DKW::System::Is99999cc()) {
        const ControllerType controllerType = controllerHolder.curController->GetType();
        const u16 inputs = controllerHolder.inputStates[0].buttonRaw;
        u16 inputsMask = 0x700;

        switch(controllerType) {
            case(NUNCHUCK):
                inputsMask = 0xC04;
                break;
            case(CLASSIC):
                inputsMask = 0x250;
                break;
            case(GCN):
                inputsMask = 0x320;
                break;
        }
        if((inputs & inputsMask) == inputsMask) controllerHolder.inputStates[0].buttonActions |= 0x10;
    }
}

static void CalcBrakeDrifting() {
    const SectionPad& pad = SectionMgr::sInstance->pad;
    for(int hudSlotId = 0; hudSlotId < 4; ++hudSlotId) {
        Input::ControllerHolder* controllerHolder = pad.GetControllerHolder(hudSlotId);
        if(controllerHolder != nullptr) EnableBrakeDrifting(*controllerHolder);
    }
}
static RaceFrameHook BrakeDriftingCheck(CalcBrakeDrifting);

void FixGhostBrakeDrifting(Input::GhostWriter* writer, u16 buttonActions, u8 quantisedStickX,
    u8 quantisedStickY, u8 motionControlFlickUnmirrored) {
    register Input::ControllerHolder* controllerHolder;
    asm(mr controllerHolder, r30;);
    EnableBrakeDrifting(*controllerHolder);
    writer->WriteFrame(controllerHolder->inputStates[0].buttonActions & ~0x20, quantisedStickX, quantisedStickY, motionControlFlickUnmirrored);
}
kmCall(0x80521828, FixGhostBrakeDrifting);


bool IsBrakeDrifting(const Kart::Status& status) {
    bool NoBrakeDrift = Pulsar::DKWSETTING_150_BRAKEDRIFT_ON;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    const GameMode gameMode = Racedata::sInstance->menusScenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
       NoBrakeDrift = System::sInstance->IsContext(Pulsar::PULSAR_BDRIFTING) ? Pulsar::DKWSETTING_150_BRAKEDRIFT_OFF : Pulsar::DKWSETTING_150_BRAKEDRIFT_ON;
}
    if(NoBrakeDrift == Pulsar::DKWSETTING_150_BRAKEDRIFT_OFF || System::sInstance->IsContext(PULSAR_MODE_OTT) && !System::sInstance->IsContext(PULSAR_200) || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is400cc() || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is99999cc() || mode == MODE_TIME_TRIAL && !System::sInstance->IsContext(PULSAR_200) || mode == MODE_GHOST_RACE && !System::sInstance->IsContext(PULSAR_200)){}
    else if(static_cast<Pulsar::DKWSettingBrakeDrift>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW1), Pulsar::SETTINGDKW_BRAKEDRIFT)) == Pulsar::DKWSETTING_BRAKEDRIFT_ENABLED || System::sInstance->IsContext(PULSAR_200) || DKW::System::Is400cc() || DKW::System::Is99999cc()) {
        u32 bitfield0 = status.bitfield0;
        const Input::ControllerHolder& controllerHolder = status.link->GetControllerHolder();
        if((bitfield0 & 0x40000) != 0 && (bitfield0 & 0x1F) == 0xF && (bitfield0 & 0x80100000) == 0
            && (controllerHolder.inputStates[0].buttonActions & 0x10) != 0x0) {
            return true;
        }
    }
    return false;
}

void BrakeDriftingAcceleration(Kart::Movement& movement) {
    movement.UpdateKartSpeed();
    if(IsBrakeDrifting(*movement.pointers->kartStatus)) movement.acceleration = brakeDriftingDeceleration; //JUMP_PAD|RAMP_BOOST|BOOST
}
kmCall(0x80579910, BrakeDriftingAcceleration);

asmFunc BrakeDriftingSoundWrapper() {
    ASM(
        nofralloc;
    mflr r27;
    mr r28, r3;
    mr r30, r4;
    mr r3, r28;
    bl IsBrakeDrifting;
    lwz r0, 0x4 (r30);
    cmpwi r3, 0;
    beq + normal;
    li r0, 2;
normal:
    mtlr r27;
    mr r3, r28;
    rlwinm r27, r0, 30, 31, 31;
    rlwinm r28, r0, 31, 31, 31;
    rlwinm r30, r0, 0, 31, 31;
    blr;
        )
}
kmCall(0x806faff8, BrakeDriftingSoundWrapper);

kmWrite32(0x80698f88, 0x60000000);
static int BrakeEffectBikes(Effects::Player& effects) {
    bool NoBrakeDrift = Pulsar::DKWSETTING_150_BRAKEDRIFT_ON;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    const GameMode gameMode = Racedata::sInstance->menusScenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
       NoBrakeDrift = System::sInstance->IsContext(Pulsar::PULSAR_BDRIFTING) ? Pulsar::DKWSETTING_150_BRAKEDRIFT_OFF : Pulsar::DKWSETTING_150_BRAKEDRIFT_ON;
}
    const Kart::Player* kartPlayer = effects.kartPlayer;
    if(NoBrakeDrift == Pulsar::DKWSETTING_150_BRAKEDRIFT_OFF || System::sInstance->IsContext(PULSAR_MODE_OTT) && !System::sInstance->IsContext(PULSAR_200) || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is400cc() || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is99999cc() || mode == MODE_TIME_TRIAL && !System::sInstance->IsContext(PULSAR_200) || mode == MODE_GHOST_RACE && !System::sInstance->IsContext(PULSAR_200)){}
    else if(static_cast<Pulsar::DKWSettingBrakeDrift>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW1), Pulsar::SETTINGDKW_BRAKEDRIFT)) == Pulsar::DKWSETTING_BRAKEDRIFT_ENABLED || System::sInstance->IsContext(PULSAR_200) || DKW::System::Is400cc() || DKW::System::Is99999cc()) {
        if(IsBrakeDrifting(*kartPlayer->pointers.kartStatus)) effects.CreateAndUpdateEffectsByIdxVelocity(effects.bikeDriftEffects, 25, 26, 1);
        else effects.FollowFadeEffectsByIdxVelocity(effects.bikeDriftEffects, 25, 26, 1);
    }
    return kartPlayer->GetDriftState();
}
kmCall(0x80698f8c, BrakeEffectBikes);

kmWrite32(0x80698048, 0x60000000);
static int BrakeEffectKarts(Effects::Player& effects) {
    bool NoBrakeDrift = Pulsar::DKWSETTING_150_BRAKEDRIFT_ON;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    const GameMode gameMode = Racedata::sInstance->menusScenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
       NoBrakeDrift = System::sInstance->IsContext(Pulsar::PULSAR_BDRIFTING) ? Pulsar::DKWSETTING_150_BRAKEDRIFT_OFF : Pulsar::DKWSETTING_150_BRAKEDRIFT_ON;
}
    Kart::Player* kartPlayer = effects.kartPlayer;
    if(NoBrakeDrift == Pulsar::DKWSETTING_150_BRAKEDRIFT_OFF || System::sInstance->IsContext(PULSAR_MODE_OTT) && !System::sInstance->IsContext(PULSAR_200) || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is400cc() || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is99999cc() || mode == MODE_TIME_TRIAL && !System::sInstance->IsContext(PULSAR_200) || mode == MODE_GHOST_RACE && !System::sInstance->IsContext(PULSAR_200)){}
    else if(static_cast<Pulsar::DKWSettingBrakeDrift>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW1), Pulsar::SETTINGDKW_BRAKEDRIFT)) == Pulsar::DKWSETTING_BRAKEDRIFT_ENABLED || System::sInstance->IsContext(PULSAR_200) || DKW::System::Is400cc() || DKW::System::Is99999cc()) {
        if(IsBrakeDrifting(*kartPlayer->pointers.kartStatus)) effects.CreateAndUpdateEffectsByIdxVelocity(effects.kartDriftEffects, 34, 36, 1);
        else effects.FollowFadeEffectsByIdxVelocity(effects.kartDriftEffects, 34, 36, 1);
    }
    return kartPlayer->GetDriftState();
}
kmCall(0x8069804c, BrakeEffectKarts);


static void FastFallingBody(Kart::Status& status, Kart::Physics& physics) { //weird thing 0x96 padding byte used
    bool NoFallFast = Pulsar::DKWSETTING_150_FALLFASTON;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    const GameMode gameMode = Racedata::sInstance->menusScenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
       NoFallFast = System::sInstance->IsContext(Pulsar::PULSAR_FALLFAST) ? DKWSETTING_150_FALLFASTOFF : DKWSETTING_150_FALLFASTON;
}
    if(NoFallFast == Pulsar::DKWSETTING_150_FALLFASTOFF || System::sInstance->IsContext(PULSAR_MODE_OTT) && !System::sInstance->IsContext(PULSAR_200) || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is400cc() || mode == MODE_TIME_TRIAL && !System::sInstance->IsContext(PULSAR_200) || mode == MODE_GHOST_RACE && !System::sInstance->IsContext(PULSAR_200)){}
    else if(static_cast<Pulsar::DKWSettingFallFast>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW1), Pulsar::SETTINGDKW_FALLFAST)) == Pulsar::DKWSETTING_FALLFAST_ENABLED || System::sInstance->IsContext(PULSAR_200) || DKW::System::Is400cc()) {
        if((status.airtime >= 2) && (!status.bool_0x96 || (status.airtime > 19))) {
            Input::ControllerHolder& controllerHolder = status.link->GetControllerHolder();
            float input = controllerHolder.inputStates[0].stick.z <= 0.0f ? 0.0f :
                (controllerHolder.inputStates[0].stick.z + controllerHolder.inputStates[0].stick.z);
            physics.gravity -= input * fastFallingBodyGravity;
        }
    }
    status.UpdateFromInput();
}
kmCall(0x805967a4, FastFallingBody);


kmWrite32(0x8059739c, 0x38A10014); //addi r5, sp, 0x14 to align with the Vec3 on the stack
static Kart::WheelPhysicsHolder& FastFallingWheels(Kart::Sub& sub, u8 wheelIdx, Vec3& gravityVector) { //weird thing 0x96 status
    bool NoFallFast = Pulsar::DKWSETTING_150_FALLFASTON;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    const GameMode gameMode = Racedata::sInstance->menusScenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
       NoFallFast = System::sInstance->IsContext(Pulsar::PULSAR_FALLFAST) ? Pulsar::DKWSETTING_150_FALLFASTOFF : Pulsar::DKWSETTING_150_FALLFASTON;
}
    float gravity = -1.3f;
    if(NoFallFast == Pulsar::DKWSETTING_150_FALLFASTOFF || System::sInstance->IsContext(PULSAR_MODE_OTT) && !System::sInstance->IsContext(PULSAR_200) || System::sInstance->IsContext(PULSAR_MODE_OTT) && !DKW::System::Is400cc() || mode == MODE_TIME_TRIAL && !System::sInstance->IsContext(PULSAR_200) || mode == MODE_GHOST_RACE && !System::sInstance->IsContext(PULSAR_200)){}
    else if(static_cast<Pulsar::DKWSettingFallFast>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW1), Pulsar::SETTINGDKW_FALLFAST)) == Pulsar::DKWSETTING_FALLFAST_ENABLED || System::sInstance->IsContext(PULSAR_200) || DKW::System::Is400cc()) {
        Kart::Status* status = sub.kartStatus;
        if(status->airtime == 0) status->bool_0x96 = ((status->bitfield0 & 0x80) != 0) ? true : false;
        else if((status->airtime >= 2) && (!status->bool_0x96 || (status->airtime > 19))) {
            Input::ControllerHolder& controllerHolder = sub.GetControllerHolder();
            float input = controllerHolder.inputStates[0].stick.z <= 0.0f ? 0.0f
                : (controllerHolder.inputStates[0].stick.z + controllerHolder.inputStates[0].stick.z);
            gravity *= (input * fastFallingWheelGravity + 1.0f);
        }
    }
    gravityVector.y = gravity;
    return sub.GetWheelPhysicsHolder(wheelIdx);
};
kmCall(0x805973a4, FastFallingWheels);
}//namespace Race
}//namespace Pulsar