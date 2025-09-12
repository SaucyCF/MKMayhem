#include <DKW.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>

//DKW Dev Note: Code by Retro Rewind and WTP Teams, based off of code by BrawlboxGaming

namespace DKW {
void *GetCustomKartParam(ArchiveMgr *archive, ArchiveSource type, const char *name, u32 *length){
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    bool BumperKart = Pulsar::DKWSETTING_STATS_MAYHEM;
    bool RiiBalance = Pulsar::DKWSETTING_STATS_MAYHEM;

    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode == MODE_VS_RACE || mode == MODE_BATTLE){
        BumperKart = System::sInstance->IsContext(Pulsar::PULSAR_BUMPERKARTSTATS) ? Pulsar::DKWSETTING_STATS_BUMPERKARTS : Pulsar::DKWSETTING_STATS_MAYHEM;
        RiiBalance = System::sInstance->IsContext(Pulsar::PULSAR_RIIBALANCEDSTATS) ? Pulsar::DKWSETTING_STATS_RIIBALANCED : Pulsar::DKWSETTING_STATS_MAYHEM;
    }
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_WW || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_WW || mode == MODE_TIME_TRIAL || mode == MODE_GHOST_RACE) 
    {
        name = "kartParam.bin";
    }
    else if (BumperKart == Pulsar::DKWSETTING_STATS_BUMPERKARTS) 
    {
        name = "kartParamBK.bin";
    }
    else if (RiiBalance == Pulsar::DKWSETTING_STATS_RIIBALANCED) 
    {
        name = "kartParamRii.bin";
    }
    else 
    {
        name = "kartParam.bin";
    }
    return archive->GetFile(type, name, length);
}
kmCall(0x80591a30, GetCustomKartParam);

void *GetCustomItemSlot(ArchiveMgr *archive, ArchiveSource type, const char *name, u32 *length){
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    bool itemModeNone = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    bool itemModeRandom = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    bool itemModeShellShock = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    bool itemModeUnknown = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    bool itemModeRain = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    bool itemModeHotPotato = Pulsar::DKWSETTING_GAMEMODE_REGULAR;

    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode == MODE_VS_RACE || mode == MODE_BATTLE){
        itemModeRandom = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODERANDOM) ? Pulsar::DKWSETTING_GAMEMODE_RANDOM : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        itemModeShellShock = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODESHELLSHOCK) ? Pulsar::DKWSETTING_GAMEMODE_SHELLSHOCK : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        itemModeUnknown = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODEUNKNOWN) ? Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        itemModeRain = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODEITEMRAIN) ? Pulsar::DKWSETTING_GAMEMODE_ITEMRAIN : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        itemModeHotPotato = System::sInstance->IsContext(Pulsar::PULSAR_BATTLEROYALE) ? Pulsar::DKWSETTING_GAMEMODE_BATTLEROYALE : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_REGIONAL) {
        itemModeNone = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }
    if (itemModeRandom == Pulsar::DKWSETTING_GAMEMODE_REGULAR || itemModeShellShock == Pulsar::DKWSETTING_GAMEMODE_REGULAR || itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_REGULAR || itemModeRain == Pulsar::DKWSETTING_GAMEMODE_REGULAR || itemModeHotPotato == Pulsar::DKWSETTING_GAMEMODE_REGULAR)
    {
        name="ItemSlotDKW.bin";
    }
    if (itemModeRandom == Pulsar::DKWSETTING_GAMEMODE_RANDOM)
    {
        name="ItemSlotRandom.bin";
    }
    if (itemModeShellShock == Pulsar::DKWSETTING_GAMEMODE_SHELLSHOCK)
    {
        name="ItemSlotShellShock.bin";
    }
    if (itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS)
    {
        name="ItemSlotRandom.bin";
    }
    if (itemModeRain == Pulsar::DKWSETTING_GAMEMODE_ITEMRAIN)
    {
        name="ItemSlotDKW.bin";
    }
    if (itemModeHotPotato == Pulsar::DKWSETTING_GAMEMODE_BATTLEROYALE)
    {
        name="ItemSlotHP.bin";
    }
    return archive->GetFile(type, name, length);

}
kmCall(0x807bb128, GetCustomItemSlot);
kmCall(0x807bb030, GetCustomItemSlot);
kmCall(0x807bb200, GetCustomItemSlot);
kmCall(0x807bb53c, GetCustomItemSlot);
kmCall(0x807bbb58, GetCustomItemSlot);

} // namespace DKW