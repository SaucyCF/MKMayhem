#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Item/Obj/Kumo.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <PulsarSystem.hpp>
#include <Settings/SettingsParam.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <DKW.hpp>

namespace Pulsar {
namespace Race {

// Mega TC
void MegaTC(Kart::Movement& movement, int frames, int unk0, int unk1) {
    bool RandomTCs = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode ==  MODE_VS_RACE || mode == MODE_BATTLE) {{
        RandomTCs = System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM) ? Pulsar::DKWSETTING_GAMEMODE_MAYHEM : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }

        if (Pulsar::System::sInstance->IsContext(PULSAR_BATTLEROYALE)) {
            movement.ApplyLightningEffect(frames, unk0, unk1);
        } else if (System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM)) {
            Random random;
            u32 tcChance = random.NextLimited(2);
            if(tcChance == 1) {
            if(System::sInstance->IsContext(PULSAR_MEGATC)) movement.ActivateMega();
            else movement.ApplyLightningEffect(frames, unk0, unk1);
        } else movement.ApplyLightningEffect(frames, unk0, unk1);
        } else if (System::sInstance->IsContext(PULSAR_MEGATC) && System::sInstance->IsContext(PULSAR_THUNDERCLOUD) == Pulsar::DKWSETTING_THUNDERCLOUD_MEGA) {
            movement.ActivateMega();
        } 
        else movement.ApplyLightningEffect(frames, unk0, unk1);
    } else {
        if (System::sInstance->IsContext(PULSAR_MEGATC)) {
            movement.ActivateMega();
        } else {
            movement.ApplyLightningEffect(frames, unk0, unk1);
        }
    }
}
kmCall(0x80580630, MegaTC);

void LoadCorrectTCBRRES(Item::ObjKumo& objKumo, const char* mdlName, const char* shadowSrc, u8 whichShadowListToUse,
    Item::Obj::AnmParam* anmParam) {
    bool RandomTCs = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode ==  MODE_VS_RACE || mode == MODE_BATTLE) {{
        RandomTCs = System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM) ? Pulsar::DKWSETTING_GAMEMODE_MAYHEM : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }

        if (Pulsar::System::sInstance->IsContext(PULSAR_BATTLEROYALE)) {
        objKumo.LoadGraphics("PotatoTC.brres", mdlName, shadowSrc, 1, anmParam,
            static_cast<nw4r::g3d::ScnMdl::BufferOption>(0), nullptr, 0);
        } else if (System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM)) {
        objKumo.LoadGraphics("RandomTC.brres", mdlName, shadowSrc, 1, anmParam,
            static_cast<nw4r::g3d::ScnMdl::BufferOption>(0), nullptr, 0);
        } else if (System::sInstance->IsContext(PULSAR_MEGATC) && System::sInstance->IsContext(PULSAR_THUNDERCLOUD) == Pulsar::DKWSETTING_THUNDERCLOUD_SHRINK) {
            objKumo.LoadGraphicsImplicitBRRES(mdlName, shadowSrc, 1, anmParam, static_cast<nw4r::g3d::ScnMdl::BufferOption>(0), nullptr);
        } else if (System::sInstance->IsContext(PULSAR_MEGATC) && System::sInstance->IsContext(PULSAR_THUNDERCLOUD) == Pulsar::DKWSETTING_THUNDERCLOUD_MEGA) {
            objKumo.LoadGraphics("megaTC.brres", mdlName, shadowSrc, 1, anmParam,
                static_cast<nw4r::g3d::ScnMdl::BufferOption>(0), nullptr, 0);        
        }
    } else {
        if (System::sInstance->IsContext(PULSAR_MEGATC)) {
            objKumo.LoadGraphics("megaTC.brres", mdlName, shadowSrc, 1, anmParam,
                static_cast<nw4r::g3d::ScnMdl::BufferOption>(0), nullptr, 0);
        } else if (System::sInstance->IsContext(PULSAR_MEGATC) && System::sInstance->IsContext(PULSAR_THUNDERCLOUD) == Pulsar::DKWSETTING_THUNDERCLOUD_SHRINK) {
            objKumo.LoadGraphicsImplicitBRRES(mdlName, shadowSrc, 1, anmParam, static_cast<nw4r::g3d::ScnMdl::BufferOption>(0), nullptr);
        }
    }
}
kmCall(0x807af568, LoadCorrectTCBRRES);

} // namespace Race
} // namespace Pulsar