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
  const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
  const GameMode mode = scenario.settings.gamemode;
  if(!System::sInstance->IsContext(PULSAR_CT)) roulette->unknown_0x24 = (u32) item;
}
kmCall(0x807a9b28, NoBulletBillIcon);

}// namespace Race
}// namespace Pulsar