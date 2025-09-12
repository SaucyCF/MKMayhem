#include <kamek.hpp>
#include <MarioKartWii/Item/ItemPlayer.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <PulsarSystem.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <DKW.hpp>

namespace Pulsar {
namespace Race {

//No Bullet Bill Icon by Gabriela.
void NoBulletBillIcon(Item::PlayerRoulette *roulette, ItemId item) {
  bool BulletBillIcon = Pulsar::DKWSETTING_BULLETICON_ENABLED;
  const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
  const GameMode mode = scenario.settings.gamemode;
  if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode ==  MODE_VS_RACE || mode == MODE_BATTLE) {
        BulletBillIcon = System::sInstance->IsContext(Pulsar::PULSAR_BULLETICON) ? Pulsar::DKWSETTING_BULLETICON_ENABLED : Pulsar::DKWSETTING_BULLETICON_DISABLED;
    }

  if(!System::sInstance->IsContext(PULSAR_CT) || BulletBillIcon != Pulsar::DKWSETTING_BULLETICON_DISABLED) roulette->unknown_0x24 = (u32) item;
}
kmCall(0x807a9b28, NoBulletBillIcon);

}// namespace Race
}// namespace Pulsar