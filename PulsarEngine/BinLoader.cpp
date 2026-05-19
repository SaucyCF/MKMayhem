#include <DKW.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>

//DKW Dev Note: Code by Retro Rewind and WTP Teams, based off of code by BrawlboxGaming

// Compiled ItemSlot binary built from C++ arrays (see ItemProbabilities.hpp)
extern "C" u8  compiledItemSlotBin[];
extern "C" u32 compiledItemSlotLen;
extern "C" void BuildItemSlotBinary();
extern "C" u8  compiledUnknownItemsBin[];
extern "C" u32 compiledUnknownItemsLen;
extern "C" void BuildUnknownItemsBinary();

namespace DKW {
void *GetCustomKartParam(ArchiveMgr *archive, ArchiveSource type, const char *name, u32 *length){
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    bool BumperKart = Pulsar::DKWSETTING_GAMEMODE_REGULAR;

    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode == MODE_VS_RACE || mode == MODE_BATTLE){
    }
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_WW || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_WW || mode == MODE_TIME_TRIAL || mode == MODE_GHOST_RACE) 
    {
        name = "kartParam.bin";
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
    bool itemModeUnknown = Pulsar::DKWSETTING_GAMEMODE_REGULAR;

    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||  RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE){
        itemModeUnknown = System::sInstance->IsContext(Pulsar::PULSAR_MODE_UNKNOWN) ? Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_REGIONAL) {
        itemModeNone = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }

    if (itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS) {
        if (compiledUnknownItemsLen == 0) BuildUnknownItemsBinary();
        if (length) *length = compiledUnknownItemsLen;
        return compiledUnknownItemsBin;
    }
    else {
        if (compiledItemSlotLen == 0) BuildItemSlotBinary();
        if (length) *length = compiledItemSlotLen;
        return compiledItemSlotBin;
    }
}
kmCall(0x807bb128, GetCustomItemSlot);
kmCall(0x807bb030, GetCustomItemSlot);
kmCall(0x807bb200, GetCustomItemSlot);
kmCall(0x807bb53c, GetCustomItemSlot);
kmCall(0x807bbb58, GetCustomItemSlot);
kmCall(0x807bbdd4, GetCustomItemSlot);
kmCall(0x807bbf50, GetCustomItemSlot);

} // namespace DKW