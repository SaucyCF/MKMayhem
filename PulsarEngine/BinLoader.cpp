#include <DKW.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>

//DKW Dev Note: Code by Retro Rewind and WTP Teams, based off of code by BrawlboxGaming

namespace DKW {
void *GetCustomKartParam(ArchiveMgr *archive, ArchiveSource type, const char *name, u32 *length){
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    bool BumperKart = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    bool RiiBalance = Pulsar::DKWSETTING_GAMEMODE_REGULAR;

    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode == MODE_VS_RACE || mode == MODE_BATTLE){
        BumperKart = System::sInstance->IsContext(Pulsar::PULSAR_MODE_BUMPERKARTS) ? Pulsar::DKWSETTING_GAMEMODE_BUMPERKARTS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        RiiBalance = System::sInstance->IsContext(Pulsar::PULSAR_MODE_RIIBALANCED) ? Pulsar::DKWSETTING_GAMEMODE_RIIBALANCED : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_WW || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_WW || mode == MODE_TIME_TRIAL || mode == MODE_GHOST_RACE) 
    {
        name = "kartParam.bin";
    }
    else if (BumperKart == Pulsar::DKWSETTING_GAMEMODE_BUMPERKARTS) 
    {
        name = "kartParamBK.bin";
    }
    else if (RiiBalance == Pulsar::DKWSETTING_GAMEMODE_RIIBALANCED) 
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
    bool itemModeUnknown = Pulsar::DKWSETTING_GAMEMODE_REGULAR;

    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||  RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE){
        itemModeUnknown = System::sInstance->IsContext(Pulsar::PULSAR_MODE_UNKNOWN) ? Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_REGIONAL) {
        itemModeNone = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }
    if (itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_REGULAR)
    {
        name="ItemSlotDKW.bin";
    }
    if (itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS)
    {
        name="ItemSlotRandom.bin";
    }
    else
    {
        name="ItemSlotDKW.bin";
    }
    return archive->GetFile(type, name, length);

}
kmCall(0x807bb128, GetCustomItemSlot);
kmCall(0x807bb030, GetCustomItemSlot);
kmCall(0x807bb200, GetCustomItemSlot);
kmCall(0x807bb53c, GetCustomItemSlot);
kmCall(0x807bbb58, GetCustomItemSlot);
kmCall(0x807bbdd4, GetCustomItemSlot);
kmCall(0x807bbf50, GetCustomItemSlot);

void KartParamCheck() { // Remove From Public Pages
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    bool BumperKart = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    bool RiiBalance = Pulsar::DKWSETTING_GAMEMODE_REGULAR;

    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode == MODE_VS_RACE || mode == MODE_BATTLE){
        BumperKart = System::sInstance->IsContext(Pulsar::PULSAR_MODE_BUMPERKARTS) ? Pulsar::DKWSETTING_GAMEMODE_BUMPERKARTS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        RiiBalance = System::sInstance->IsContext(Pulsar::PULSAR_MODE_RIIBALANCED) ? Pulsar::DKWSETTING_GAMEMODE_RIIBALANCED : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }

    const char* name = nullptr;
    const unsigned char* expectedHash = nullptr;

    static const unsigned char HASH_DEFAULT[32] = {
        0xfa,0x35,0x09,0xc5,0x3a,0x7f,0x31,0xd8,0x7d,0xb6,0xc6,0x63,0x2d,0x88,0xcb,0xef,
        0xe6,0x8b,0xb7,0xcf,0xab,0xf8,0xcb,0xb9,0xfd,0xbd,0x3c,0x0b,0x70,0x15,0xc2,0xf3
    };
    static const unsigned char HASH_BUMPERKART[32] = {
        0x08,0xb4,0xa2,0x5e,0x06,0x14,0x52,0x41,0x91,0x65,0x92,0x0d,0x93,0x3a,0x6c,0xb3,
        0x3c,0x2b,0xc5,0x8c,0x58,0xc6,0x13,0x23,0x04,0xd5,0x65,0xbe,0x98,0xf2,0xa9,0xba
    };
    static const unsigned char HASH_RIIBALANCED[32] = {
        0xba,0x18,0x5c,0xb1,0xc2,0x0a,0xfa,0xc4,0x7a,0x20,0x88,0x19,0x2c,0x05,0x01,0xc9,
        0x66,0x0d,0x9f,0x5c,0x36,0xa5,0x2b,0xd1,0xb8,0x36,0xc5,0x2f,0x40,0xb9,0x59,0xd4
    };

    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_WW || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_BT_WW || mode == MODE_TIME_TRIAL || mode == MODE_GHOST_RACE) 
    {
        name = "kartParam.bin";
        expectedHash = HASH_DEFAULT;
    }
    else if (BumperKart == Pulsar::DKWSETTING_GAMEMODE_BUMPERKARTS) 
    {
        name = "kartParamBK.bin";
        expectedHash = HASH_BUMPERKART;
    }
    else if (RiiBalance == Pulsar::DKWSETTING_GAMEMODE_RIIBALANCED) 
    {
        name = "kartParamRii.bin";
        expectedHash = HASH_RIIBALANCED;
    }
    else 
    {
        name = "kartParam.bin";
        expectedHash = HASH_DEFAULT;
    }
    ArchiveMgr* archive = ArchiveMgr::sInstance;
    ArchiveSource type = (ArchiveSource)0;
    u32 length = 0;
    void* fileData = archive->GetFile(type, name, &length);
    if (!fileData || length == 0) return;

    unsigned char hash[32];
    extern void SHA256(const void* data, size_t len, unsigned char* out);
    SHA256(fileData, length, hash);

    for (int i = 0; i < 32; ++i) {
        if (hash[i] != expectedHash[i]) {
            Pulsar::Debug::FatalError("Error: Incorrect KartParam Hash");
            return;
        }
    }
}
static PageLoadHook KartParamAntiCheat(KartParamCheck); // Remove From Public Pages

void ItemSlotCheck() { // Remove From Public Pages
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    bool itemModeNone = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    bool itemModeUnknown = Pulsar::DKWSETTING_GAMEMODE_REGULAR;

    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE){
        itemModeUnknown = System::sInstance->IsContext(Pulsar::PULSAR_MODE_UNKNOWN) ? Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }

    const char* name = nullptr;
    const unsigned char* expectedHash = nullptr;

    static const unsigned char HASH_RANDOM[32] = {
        0x11,0x36,0x96,0x91,0x4c,0x23,0xdd,0xfc,0x4d,0x13,0x0f,0xca,0x10,0xcc,0x51,0x3e,
        0x8b,0xb5,0x18,0x69,0x61,0xa0,0x0d,0x3f,0x76,0x4b,0xd6,0xde,0x61,0x69,0xb3,0x51
    };
    static const unsigned char HASH_DKW[32] = {
        0x01,0x67,0xca,0x98,0x64,0x0f,0xdc,0x7d,0x0e,0x6e,0xe0,0x8f,0x32,0x83,0x9d,0xd1,
        0x50,0x7c,0x9b,0xd5,0x71,0xe2,0x90,0x8b,0x2b,0x60,0x7e,0x99,0x5a,0xe7,0x2a,0x53
    };

    if (itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_REGULAR)
    {
        name="ItemSlotDKW.bin";
        expectedHash = HASH_DKW;
    } else if (itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS) {
        name = "ItemSlotRandom.bin";
        expectedHash = HASH_RANDOM;
    } else {
        name = "ItemSlotDKW.bin";
        expectedHash = HASH_DKW;
    }

    ArchiveMgr* archive = ArchiveMgr::sInstance;
    ArchiveSource type = (ArchiveSource)0;
    u32 length = 0;
    void* fileData = archive->GetFile(type, name, &length);
    if (!fileData || length == 0) return;

    unsigned char hash[32];
    extern void SHA256(const void* data, size_t len, unsigned char* out);
    SHA256(fileData, length, hash);

    for (int i = 0; i < 32; ++i) {
        if (hash[i] != expectedHash[i]) {
            Pulsar::Debug::FatalError("Error: Incorrect Item Hash");
            return;
        }
    }
}
static PageLoadHook ItemSlotAntiCheat(ItemSlotCheck); // Remove From Public Pages

} // namespace DKW