#include <kamek.hpp>
#include <Settings/SettingsParam.hpp>
#include <DKW.hpp>
#include <PulsarSystem.hpp>
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>

namespace Pulsar{
namespace Settings{

void Settings() {
const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
const GameMode mode = scenario.settings.gamemode;
MKMSnaking = 0x0000;
MKMRanked = 0x0000;
MKMayhem = 0x0000;
MKMItemStatus = 0x0000;
MKMBattleRoyale = 0x0000;
MKMTCToggle = 0x0000;
MKMItemBox = 0x0000;
MKMStarDance = 0x0000;
MKMMegaFOV = 0x0000;
MKMRemoveLuma = 0x0000;
MKMHybrid = 0x0001;
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_SNAKING) == DKWSETTING_SNAKING_ENABLED && mode == MODE_VS_RACE || System::sInstance->IsContext(Pulsar::PULSAR_CT) && System::sInstance->IsContext(Pulsar::PULSAR_SNAKING) || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_REGIONAL){
    MKMSnaking = 0x0001;
}
if (Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGDKW_RANKED_FROOMS) == DKWSETTING_RANKEDFROOMS_ENABLED && mode == MODE_VS_RACE || System::sInstance->IsContext(Pulsar::PULSAR_CT) && System::sInstance->IsContext(Pulsar::PULSAR_RANKED) && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_VS_REGIONAL && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_BT_REGIONAL){
    MKMRanked = 0x0001;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_MAYHEM_CODES) == DKWSETTING_MAYHEM_ENABLED && mode == MODE_VS_RACE || System::sInstance->IsContext(Pulsar::PULSAR_CT) && System::sInstance->IsContext(Pulsar::PULSAR_CODES) || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_REGIONAL){
    MKMayhem = 0x0001;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_ITEMSTATUS) == DKWSETTING_ITEMSTATUS_ENABLED && mode == MODE_VS_RACE || System::sInstance->IsContext(Pulsar::PULSAR_CT) && System::sInstance->IsContext(Pulsar::PULSAR_ITEMSTATUS) && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_VS_REGIONAL && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_BT_REGIONAL){
    MKMItemStatus = 0x0001;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_BATTLEROYALE && mode == MODE_VS_RACE || System::sInstance->IsContext(Pulsar::PULSAR_CT) && System::sInstance->IsContext(Pulsar::PULSAR_BATTLEROYALE) && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_VS_REGIONAL && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_BT_REGIONAL){
    MKMBattleRoyale = 0x0001;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ITEMBOXRESPAWN) == DKWSETTING_ITEMBOX_FASTRESPAWN && mode == MODE_VS_RACE || System::sInstance->IsContext(Pulsar::PULSAR_CT) && System::sInstance->IsContext(Pulsar::PULSAR_ITEMBOXFAST) && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_VS_REGIONAL && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_BT_REGIONAL){
    MKMItemBox = 0x0001;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ITEMBOXRESPAWN) == DKWSETTING_ITEMBOX_INSTANTRESPAWN && mode == MODE_VS_RACE || System::sInstance->IsContext(Pulsar::PULSAR_CT) && System::sInstance->IsContext(Pulsar::PULSAR_ITEMBOXINSTANT) && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_VS_REGIONAL && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_BT_REGIONAL){
    MKMItemBox = 0x0002;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_TCTOGGLE) == DKWSETTING_TCTOGGLE_ENABLED){
    MKMTCToggle = 0x0001;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW5, SETTINGDKW_STARANIM) == DKWSETTING_STARANIM_ENABLED) {
    MKMStarDance = 0x0001;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW5, SETTINGDKW_MEGAFOV) == DKWSETTING_MEGAFOV_ENABLED) {
    MKMMegaFOV = 0x0001;
}
if (Settings::Mgr::Get().GetUserSettingValue(Settings::SETTINGSTYPE_DKW5, SETTINGDKW_NOLUMA) == DKWSETTING_NOLUMA_ENABLED) {
    MKMRemoveLuma = 0x0001;
}
if (mode == MODE_TIME_TRIAL || mode == MODE_GHOST_RACE) {
    MKMHybrid = 0x0000;
}
}
static RaceLoadHook PatchSettings(Settings);

} // namespace Settings
} // namespace Pulsar