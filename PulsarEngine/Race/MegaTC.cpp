#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Item/Obj/Kumo.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <PulsarSystem.hpp>
#include <Settings/SettingsParam.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <Race/Boo.hpp>
#include <DKW.hpp>

namespace Pulsar {
namespace Race {

static void ActivateBoo(Kart::Movement& movement) {
    const u8 playerId = movement.GetPlayerIdx();
    if (playerId >= 12) return;

    Item::Manager* itemManager = Item::Manager::sInstance;
    if (itemManager == nullptr || itemManager->players == nullptr || playerId >= itemManager->playerCount) return;

    UseBoo(itemManager->players[playerId]);
}

// Mega TC
void MegaTC(Kart::Movement& movement, int frames, int unk0, int unk1) {
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    Random random;
    u32 tcChance = random.NextLimited(6);
        if(tcChance == 1){
            movement.ApplyLightningEffect(frames, unk0, unk1);
        }
        else if (tcChance == 2){
            movement.ActivateMega();
        }
        else if (tcChance == 3){
            movement.ActivateMushroom();
        }
        else if (tcChance == 4){
            movement.ActivateStar();
        }
        else if (tcChance == 5){
            ActivateBoo(movement);
        }
        else if (tcChance == 6){
            movement.ApplyInk(true);
        } else {
            movement.ApplyLightningEffect(frames, unk0, unk1);
    }
}
kmCall(0x80580630, MegaTC);

void LoadCorrectTCBRRES(Item::ObjKumo& objKumo, const char* mdlName, const char* shadowSrc, u8 whichShadowListToUse,
    Item::Obj::AnmParam* anmParam) {
    bool RandomTCs = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
        if (System::sInstance->IsContext(PULSAR_CT)) {
            objKumo.LoadGraphics("megaTC.brres", mdlName, shadowSrc, 1, anmParam,
            static_cast<nw4r::g3d::ScnMdl::BufferOption>(0), nullptr, 0);        
        } else {
            objKumo.LoadGraphicsImplicitBRRES(mdlName, shadowSrc, 1, anmParam, static_cast<nw4r::g3d::ScnMdl::BufferOption>(0), nullptr);
        }
}
kmCall(0x807af568, LoadCorrectTCBRRES);

} // namespace Race
} // namespace Pulsar