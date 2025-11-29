#include <kamek.hpp>
#include <Settings/SettingsParam.hpp>
#include <MarioKartWii/Race/Racedata.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <Settings/UI/SettingsPanel.hpp>
#include <Settings/Settings.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <PulsarSystem.hpp>
#include <DKW.hpp>
#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <Network/SHA256.hpp>

namespace DKW {
    Pulsar::System *System::Create() {
        return new System();
}
Pulsar::System::Inherit CreateDKW(System::Create);

bool System::Is50cc() {
  const Pulsar::Settings::Mgr& settings = Pulsar::Settings::Mgr::Get();
  return Racedata::sInstance->racesScenario.settings.engineClass == CC_50 && settings.GetSettingValue(Pulsar::Settings::SETTINGSTYPE_MISC2, Pulsar::MISC_CUSTOMCC) == Pulsar::DKWSETTING_CUSTOMCC_50;
}
bool System::Is100cc() {
  const Pulsar::Settings::Mgr& settings = Pulsar::Settings::Mgr::Get();
  return Racedata::sInstance->racesScenario.settings.engineClass == CC_50 && settings.GetSettingValue(Pulsar::Settings::SETTINGSTYPE_MISC2, Pulsar::MISC_CUSTOMCC) == Pulsar::DKWSETTING_CUSTOMCC_100;
}
bool System::Is400cc() {
  const Pulsar::Settings::Mgr& settings = Pulsar::Settings::Mgr::Get();
  return Racedata::sInstance->racesScenario.settings.engineClass == CC_50 && settings.GetSettingValue(Pulsar::Settings::SETTINGSTYPE_MISC2, Pulsar::MISC_CUSTOMCC) == Pulsar::DKWSETTING_CUSTOMCC_400;
}
bool System::Is99999cc() {
  const Pulsar::Settings::Mgr& settings = Pulsar::Settings::Mgr::Get();
  return Racedata::sInstance->racesScenario.settings.engineClass == CC_50 && settings.GetSettingValue(Pulsar::Settings::SETTINGSTYPE_MISC2, Pulsar::MISC_CUSTOMCC) == Pulsar::DKWSETTING_CUSTOMCC_99999;
}

System::WeightClass System::GetWeightClass(const CharacterId id){
    switch (id)
    {
        case BABY_MARIO:
        case BABY_LUIGI:
        case BABY_PEACH:
        case BABY_DAISY:
        case TOAD:
        case TOADETTE:
        case KOOPA_TROOPA:
        case DRY_BONES:
            return LIGHTWEIGHT;
        case MARIO:
        case LUIGI:
        case PEACH:
        case DAISY:
        case YOSHI:
        case BIRDO:
        case DIDDY_KONG:
        case BOWSER_JR:
            return MEDIUMWEIGHT;
        case WARIO:
        case WALUIGI:
        case DONKEY_KONG:
        case BOWSER:
        case KING_BOO:
        case ROSALINA:
        case FUNKY_KONG:
        case DRY_BOWSER:
            return HEAVYWEIGHT;
        default:
            return MIIS;
    }
}

void SHA256(const void* data, unsigned long len, unsigned char* out) {
        SHA256Context ctx;
        SHA256Init(&ctx);
        SHA256Update(&ctx, data, len);
        u8* digest = SHA256Final(&ctx);
        for (int i = 0; i < SHA256_DIGEST_SIZE; ++i) {
            out[i] = digest[i];
        }
    }
} // namespace DKW