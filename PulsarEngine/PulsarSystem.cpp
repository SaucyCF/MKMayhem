#include <core/RK/RKSystem.hpp>
#include <core/nw4r/ut/Misc.hpp>
#include <MarioKartWii/Scene/RootScene.hpp>
#include <MarioKartWii/GlobalFunctions.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <PulsarSystem.hpp>
#include <Extensions/LECODE/LECODEMgr.hpp>
#include <Gamemodes/KO/KOMgr.hpp>
#include <Gamemodes/KO/KOHost.hpp>
#include <Gamemodes/LapKO/LapKOMgr.hpp>
#include <Gamemodes/OnlineTT/OnlineTT.hpp>
#include <Settings/Settings.hpp>
#include <Config.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <core/egg/DVD/DvdRipper.hpp>
#include <MarioKartWii/UI/Page/Other/FriendList.hpp>
#include <Network/PacketExpansion.hpp>
#include <include/c_string.h>

namespace Pulsar {

System* System::sInstance = nullptr;
System::Inherit* System::inherit = nullptr;

void System::CreateSystem() {
    if(sInstance != nullptr) return;
    EGG::Heap* heap = RKSystem::mInstance.EGGSystem;
    const EGG::Heap* prev = heap->BecomeCurrentHeap();
    System* system;
    if(inherit != nullptr) {
        system = inherit->create();
    }
    else system = new System();
    System::sInstance = system;
    ConfigFile& conf = ConfigFile::LoadConfig();
    system->Init(conf);
    prev->BecomeCurrentHeap();
    conf.Destroy();
}
//kmCall(0x80543bb4, System::CreateSystem);
BootHook CreateSystem(System::CreateSystem, 0);

System::System() :
    heap(RKSystem::mInstance.EGGSystem), taskThread(EGG::TaskThread::Create(8, 0, 0x4000, this->heap)),
    //Modes
    koMgr(nullptr), lapKoMgr(nullptr), ottHideNames(false) {
}

void System::Init(const ConfigFile& conf) {
    IOType type = IOType_ISO;
    s32 ret = IO::OpenFix("file", IOS::MODE_NONE);

    if(ret >= 0) {
        type = IOType_RIIVO;
        IOS::Close(ret);
    }
    else {
        ret = IO::OpenFix("/dev/dolphin", IOS::MODE_NONE);
        if(ret >= 0) {
            type = IOType_DOLPHIN;
            IOS::Close(ret);
        }
    }
    strncpy(this->modFolderName, conf.header.modFolderName, IOS::ipcMaxFileName);
    static char* pulMagic = reinterpret_cast<char*>(0x800017CC);
    strcpy(pulMagic, "PUL2");

    //InitInstances
    CupsConfig::sInstance = new CupsConfig(conf.GetSection<CupsHolder>());
    this->info.Init(conf.GetSection<InfoHolder>().info);
    this->InitIO(type);
    this->InitSettings(&conf.GetSection<CupsHolder>().trophyCount[0]);

    //Initialize last selected cup and courses
    const PulsarCupId last = Settings::Mgr::sInstance->GetSavedSelectedCup();
    CupsConfig* cupsConfig = CupsConfig::sInstance;
    cupsConfig->SetLayout();
    if(last != -1 && cupsConfig->IsValidCup(last) && cupsConfig->GetTotalCupCount() > 8) {
        cupsConfig->lastSelectedCup = last;
        cupsConfig->SetSelected(cupsConfig->ConvertTrack_PulsarCupToTrack(last, 0));
        cupsConfig->lastSelectedCupButtonIdx = last & 1;
    }

    //Track blocking 
    u32 trackBlocking = 16; //this->info.GetTrackBlocking();
    this->netMgr.lastTracks = new PulsarId[trackBlocking];
    for(int i = 0; i < trackBlocking; ++i) this->netMgr.lastTracks[i] = PULSARID_NONE;
    const BMGHeader* const confBMG = &conf.GetSection<PulBMG>().header;
    this->rawBmg = EGG::Heap::alloc<BMGHeader>(confBMG->fileLength, 0x4, RootScene::sInstance->expHeapGroup.heaps[1]);
    memcpy(this->rawBmg, confBMG, confBMG->fileLength);
    this->customBmgs.Init(*this->rawBmg);
    this->AfterInit();
}

//IO
#pragma suppress_warnings on
void System::InitIO(IOType type) const {

    IO* io = IO::CreateInstance(type, this->heap, this->taskThread);
    bool ret;
    if(io->type == IOType_DOLPHIN) ret = ISFS::CreateDir("/shared2/Pulsar", 0, IOS::MODE_READ_WRITE, IOS::MODE_READ_WRITE, IOS::MODE_READ_WRITE);
    const char* modFolder = this->GetModFolder();
    ret = io->CreateFolder(modFolder);
    if(!ret && io->type == IOType_DOLPHIN) {
        char path[0x100];
        snprintf(path, 0x100, "Unable to automatically create a folder for this CT distribution\nPlease create a Pulsar folder in Dolphin Emulator/Wii/shared2", modFolder);
        Debug::FatalError(path);
    }
    char ghostPath[IOS::ipcMaxPath];
    snprintf(ghostPath, IOS::ipcMaxPath, "%s%s", modFolder, "/Ghosts");
    io->CreateFolder(ghostPath);
}
#pragma suppress_warnings reset

void System::InitSettings(const u16* totalTrophyCount) const {
    Settings::Mgr* settings = new (this->heap) Settings::Mgr;
    char settingsPath[IOS::ipcMaxPath];
    snprintf(settingsPath, IOS::ipcMaxPath, "%s/%s", this->GetModFolder(), "GameSettings.pul");
    char trophiesPath[IOS::ipcMaxPath];
    snprintf(trophiesPath, IOS::ipcMaxPath, "%s/%s", this->GetModFolder(), "Settings.pul");  // Original settings file
    settings->Init(totalTrophyCount, settingsPath, trophiesPath);
    Settings::Mgr::sInstance = settings;
}

void System::UpdateContext() {
    const RacedataSettings& racedataSettings = Racedata::sInstance->menusScenario.settings;
    const GameMode mode = racedataSettings.gamemode;
    this->ottVoteState = OTT::COMBO_NONE;
    const Settings::Mgr& settings = Settings::Mgr::Get();
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    Network::Mgr& netMgr = this->netMgr;
    const u32 sceneId = GameScene::GetCurrent()->id;

    bool isFroom = controller->roomType == RKNet::ROOMTYPE_FROOM_HOST || controller->roomType == RKNet::ROOMTYPE_FROOM_NONHOST;
    bool isRegionalRoom = controller->roomType == RKNet::ROOMTYPE_VS_REGIONAL || controller->roomType == RKNet::ROOMTYPE_JOINING_REGIONAL || controller->roomType == RKNet::ROOMTYPE_BT_REGIONAL;
    bool isBattle = mode == MODE_BATTLE || mode == MODE_PRIVATE_BATTLE || mode == MODE_PUBLIC_BATTLE;
    bool isBalloonBattle = isBattle && racedataSettings.battleType == BATTLE_BALLOON;
    bool isNotPublic = isFroom || controller->roomType == RKNet::ROOMTYPE_NONE;
    bool isTimeTrial = mode == MODE_TIME_TRIAL;

    bool isCT = true;
    bool isHAW = false;
    bool isKO = false;
    bool isOTT = false;
    bool isMiiHeads = settings.GetSettingValue(Settings::SETTINGSTYPE_MISC3, MISC_RADIO_MII);
    bool isLapBasedKO = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, KO_ENABLED) == KOSETTING_LAP_ENABLED && mode != MODE_TIME_TRIAL && mode != MODE_PUBLIC_VS;


    bool is200 = racedataSettings.engineClass == CC_100 && this->info.Has200cc();

    bool is50 = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, HOSTSETTING_CC_50);
    bool is100 = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, HOSTSETTING_CC_REAL100);
    bool is400 = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, HOSTSETTING_CC_400);
    bool is99999 = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, HOSTSETTING_CC_99999);
    bool isCantBrakeDrift = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_BDRIFTING) == DKWSETTING_150_BRAKEDRIFT_OFF;
    bool isCantFastFall = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_FALLFAST) == DKWSETTING_150_FALLFASTOFF;
    bool isCantAlwaysDrift = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_ALWAYSDRIFT) == DKWSETTING_ALLOW_DANYWHEREOFF;
    bool isUltras = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_ULTRAS) == DKWSETTING_ULTRAS_ENABLED;

    bool isKOFinal = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, KO_FINAL) == KOSETTING_FINAL_ALWAYS;

    bool isMayhemCodes = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_MAYHEM_CODES) == DKWSETTING_MAYHEM_ENABLED;
    bool isDisableInvisWalls = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_INVIS_WALLS) == DKWSETTING_INVISWALLS_DISABLED;
    bool isRiiBalanced = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_RIIBALANCED;
    bool isBumperKart = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_BUMPERKARTS;
    bool isItemModeUnknown = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_UNKNOWNITEMS;
    bool isItemModeRain = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_ITEMRAIN;
    bool isItemModeMayhem = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_MAYHEM;
    bool isItemModeBattleRoyale = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_BATTLEROYALE;
    bool isCharRestrictLight = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_LIGHT;
    bool isCharRestrictMedium = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_MEDIUM;
    bool isCharRestrictHeavy = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_HEAVY;
    bool isKartRestrictKart = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_KARTS;
    bool isKartRestrictBike = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_BIKES;
    bool isTransmissionVanilla = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_VANILLA;
    bool isTransmissionInsideAll = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_INSIDEALL;
    bool isTransmissionOutsideAll = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_OUTSIDEALL;

    bool isBoxSpawnFast = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_FASTSPAWN;
    bool isBoxSpawnInstant = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_INSTANTSPAWN;
    bool isBoxSpawnDisabled = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_DISABLED;
    bool isTCToggle = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_TCTOGGLE) == DKWSETTING_TCTOGGLE_ENABLED;
    bool isAllItems = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_ALLITEMS) == DKWSETTING_ALLITEMS_ENABLED;
    bool isThunderCloud = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_THUNDERCLOUD);
    bool isFlyingBlooper = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_FLYINGBLOOP);

    bool isWorldwideMKDS = settings.GetSettingValue(Settings::SETTINGSTYPE_MISC2, WW_GAMEMODE) == DKWSETTING_WWGAMEMODE_MKDS;
    bool isWorldwideItemRain = settings.GetSettingValue(Settings::SETTINGSTYPE_MISC2, WW_GAMEMODE) == DKWSETTING_WWGAMEMODE_ITEMRAIN;
    bool isWorldwideMayhem = settings.GetSettingValue(Settings::SETTINGSTYPE_MISC2, WW_GAMEMODE) == DKWSETTING_WWGAMEMODE_MAYHEM;
    bool isStartMKDS = false;
    bool isStartItemRain = false;
    bool isStartMayhem = false;

    bool isFeather = this->info.HasFeather();
    bool isUMTs = this->info.HasUMTs();
    bool isMegaTC = this->info.HasMegaTC();
    u32 newContext = 0;
    u32 newContext2 = 0;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    if(sceneId != SCENE_ID_GLOBE && controller->connectionState != RKNet::CONNECTIONSTATE_SHUTDOWN) {
        switch(controller->roomType) {
            case(RKNet::ROOMTYPE_VS_REGIONAL):
            case(RKNet::ROOMTYPE_JOINING_REGIONAL):
                isOTT = netMgr.ownStatusData == true;
                break;
            case(RKNet::ROOMTYPE_FROOM_HOST):
            case(RKNet::ROOMTYPE_FROOM_NONHOST):
                isCT = mode != MODE_BATTLE && mode != MODE_PUBLIC_BATTLE && mode != MODE_PRIVATE_BATTLE;
                newContext = netMgr.hostContext;
                newContext2 = netMgr.hostContext2;
                isKOFinal = newContext & (1 << PULSAR_KOFINAL);
                isKO = newContext & (1 << PULSAR_MODE_KO);
                isLapBasedKO = newContext & (1 << PULSAR_MODE_LAPKO);
                isMayhemCodes = newContext & (1 << PULSAR_MAYHEM);
                isBoxSpawnFast = newContext2 & (1 << PULSAR_FASTBOX);
                isBoxSpawnInstant = newContext2 & (1 << PULSAR_INSTANTBOX);
                isBoxSpawnDisabled = newContext2 & (1 << PULSAR_DISABLEBOX);
                isTCToggle = newContext & (1 << PULSAR_TCTOGGLE);
                isItemModeBattleRoyale = newContext2 & (1 << PULSAR_BATTLEROYALE);
                isItemModeMayhem = newContext2 & (1 << PULSAR_MODE_MAYHEM);
                isRiiBalanced = newContext2 & (1 << PULSAR_MODE_RIIBALANCED);
                isBumperKart = newContext2 & (1 << PULSAR_MODE_BUMPERKARTS);
                isCharRestrictLight = newContext & (1 << PULSAR_CHARRESTRICTLIGHT);
                isCharRestrictMedium = newContext & (1 << PULSAR_CHARRESTRICTMEDIUM);
                isCharRestrictHeavy = newContext & (1 << PULSAR_CHARRESTRICTHEAVY);
                isKartRestrictKart = newContext & (1 << PULSAR_KARTRESTRICT);
                isKartRestrictBike = newContext & (1 << PULSAR_BIKERESTRICT);
                isItemModeUnknown = newContext2 & (1 << PULSAR_MODE_UNKNOWN);
                isItemModeRain = newContext2 & (1 << PULSAR_MODE_ITEMRAIN);
                isUltras = newContext & (1 << PULSAR_ULTRAS);
                is50 = newContext2 & (1 << PULSAR_50);
                is100 = newContext2 & (1 << PULSAR_100);
                is400 = newContext2 & (1 << PULSAR_400);
                is99999 = newContext2 & (1 << PULSAR_99999);
                isHAW = newContext & (1 << PULSAR_HAW);
                isMiiHeads = newContext & (1 << PULSAR_MIIHEADS);
                isThunderCloud = newContext & (1 << PULSAR_THUNDERCLOUD);
                isFlyingBlooper = newContext & (1 << PULSAR_FLYINGBLOOP);
                isAllItems = newContext & (1 << PULSAR_ALLITEMS);
                isTransmissionVanilla = newContext & (1 << PULSAR_TRANSMISSIONVANILLA);
                isTransmissionInsideAll = newContext & (1 << PULSAR_TRANSMISSIONINSIDEALL);
                isTransmissionOutsideAll = newContext & (1 << PULSAR_TRANSMISSIONOUTSIDEALL);
                isCantBrakeDrift = newContext & (1 << PULSAR_BDRIFTING);
                isCantFastFall = newContext & (1 << PULSAR_FALLFAST);
                isCantAlwaysDrift = newContext & (1 << PULSAR_NODRIFTANYWHERE);
                isDisableInvisWalls = newContext & (1 << PULSAR_INVISWALLS);
                isWorldwideMKDS = newContext2 & (1 << PULSAR_WWMKDS);
                isWorldwideItemRain = newContext2 & (1 << PULSAR_WWITEMRAIN);
                isWorldwideMayhem = newContext2 & (1 << PULSAR_WWMAYHEM);
                isStartMKDS = newContext & (1 << PULSAR_STARTMKDS);
                isStartItemRain = newContext & (1 << PULSAR_STARTITEMRAIN);
                isStartMayhem = newContext & (1 << PULSAR_STARTMAYHEM);
                break;
            default: isCT = false;
        }
    }
    this->netMgr.hostContext = newContext;
    this->netMgr.hostContext2 = newContext2;

    u32 preserved = this->context & (1 << PULSAR_MAYHEM);
    u32 preserved2 = this->context2 & ((1 << PULSAR_WWMKDS) | (1 << PULSAR_WWITEMRAIN) | (1 << PULSAR_WWMAYHEM) | (1 << PULSAR_MODE_ITEMRAIN) | (1 << PULSAR_MODE_MAYHEM));

    // When entering a friend room (host/nonhost), clear any region-preserved bits
    if (controller->roomType == RKNet::ROOMTYPE_FROOM_HOST || controller->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || controller->roomType == RKNet::ROOMTYPE_NONE) {
        preserved &= ~(1 << PULSAR_MAYHEM);
        preserved2 &= ~((1 << PULSAR_MODE_ITEMRAIN) | (1 << PULSAR_MODE_MAYHEM));
    }

    u32 context = (isCT << PULSAR_CT) | (isHAW << PULSAR_HAW) | (isMiiHeads << PULSAR_MIIHEADS);
    u32 context2 = 0;
    if(isCT) { //contexts that should only exist when CTs are on
        context |= (isFeather << PULSAR_FEATHER) | (isUMTs << PULSAR_UMTS) | (isMegaTC << PULSAR_MEGATC) | (isKO << PULSAR_MODE_KO) | (isLapBasedKO << PULSAR_MODE_LAPKO) | (isUltras << PULSAR_ULTRAS) | (isCharRestrictLight << PULSAR_CHARRESTRICTLIGHT) | (isCharRestrictMedium << PULSAR_CHARRESTRICTMEDIUM) | (isCharRestrictHeavy << PULSAR_CHARRESTRICTHEAVY) | (isKartRestrictKart << PULSAR_KARTRESTRICT) | (isKartRestrictBike << PULSAR_BIKERESTRICT) | (isKOFinal << PULSAR_KOFINAL) | (isThunderCloud << PULSAR_THUNDERCLOUD) | (isFlyingBlooper << PULSAR_FLYINGBLOOP) | (isTransmissionVanilla << PULSAR_TRANSMISSIONVANILLA) | (isTransmissionInsideAll << PULSAR_TRANSMISSIONINSIDEALL) | (isTransmissionOutsideAll << PULSAR_TRANSMISSIONOUTSIDEALL) | (isCantBrakeDrift << PULSAR_BDRIFTING) | (isCantFastFall << PULSAR_FALLFAST) | (isCantAlwaysDrift << PULSAR_NODRIFTANYWHERE) | (isDisableInvisWalls << PULSAR_INVISWALLS) | (isAllItems << PULSAR_ALLITEMS) | (isMayhemCodes << PULSAR_MAYHEM) | (isTCToggle << PULSAR_TCTOGGLE) | (isStartMKDS << PULSAR_STARTMKDS) | (isStartItemRain << PULSAR_STARTITEMRAIN) | (isStartMayhem << PULSAR_STARTMAYHEM);

        context2 |= (is200 << PULSAR_200) | (is50 << PULSAR_50) | (is100 << PULSAR_100) | (is400 << PULSAR_400) |(is99999 << PULSAR_99999) | (isBumperKart << PULSAR_MODE_BUMPERKARTS) | (isRiiBalanced << PULSAR_MODE_RIIBALANCED) | (isItemModeUnknown << PULSAR_MODE_UNKNOWN) | (isItemModeRain << PULSAR_MODE_ITEMRAIN) | (isItemModeMayhem << PULSAR_MODE_MAYHEM) | (isItemModeBattleRoyale << PULSAR_BATTLEROYALE) | (isWorldwideMKDS << PULSAR_WWMKDS) | (isWorldwideItemRain << PULSAR_WWITEMRAIN) | (isWorldwideMayhem << PULSAR_WWMAYHEM) | (isBoxSpawnFast << PULSAR_FASTBOX) | (isBoxSpawnInstant << PULSAR_INSTANTBOX) | (isBoxSpawnDisabled << PULSAR_DISABLEBOX);
    }
    this->context = context | preserved;
    this->context2 = context2 | preserved2;

    //Set contexts based on region for regionals
    const u32 region = this->netMgr.region;
    if (isRegionalRoom) {
        switch (region) {
            case 0x4D:  // MKDS Worldwide
                this->context |= (1 << PULSAR_MAYHEM);
                sInstance->context2 &= ~(1 << PULSAR_MODE_ITEMRAIN);
                sInstance->context2 &= ~(1 << PULSAR_MODE_MAYHEM);
                break;

            case 0x4E:  // Item Rain Worldwide
                this->context |= (1 << PULSAR_MAYHEM);
                this->context2 |= (1 << PULSAR_MODE_ITEMRAIN);
                sInstance->context2 &= ~(1 << PULSAR_MODE_MAYHEM);
                break;

            case 0x4F:  // M4YH3M MODE Worldwide
                this->context |= (1 << PULSAR_MAYHEM);
                this->context2 |= (1 << PULSAR_MODE_MAYHEM);
                sInstance->context2 &= ~(1 << PULSAR_MODE_ITEMRAIN);
                break;
        }
    }

    //Create temp instances if needed:
    /*
    if(sceneId == SCENE_ID_RACE) {
        if(this->lecodeMgr == nullptr) this->lecodeMgr = new (this->heap) LECODE::Mgr;
    }
    else if(this->lecodeMgr != nullptr) {
        delete this->lecodeMgr;
        this->lecodeMgr = nullptr;
    }
    */

    if(isKO) {
        if(sceneId == SCENE_ID_MENU && SectionMgr::sInstance->sectionParams->onlineParams.currentRaceNumber == -1) this->koMgr = new (this->heap) KO::Mgr; //create komgr when loading the select phase of the 1st race of a froom
    }
    if(!isKO && this->koMgr != nullptr || isKO && sceneId == SCENE_ID_GLOBE) {
        delete this->koMgr;
        this->koMgr = nullptr;
    }

    if (isLapBasedKO) {
        if (this->lapKoMgr == nullptr) {
            this->lapKoMgr = new (this->heap) LapKO::Mgr;
        }
    } else if (this->lapKoMgr != nullptr) {
        delete this->lapKoMgr;
        this->lapKoMgr = nullptr;
    }
}

void System::UpdateContextWrapper() {
    System::sInstance->UpdateContext();
}

static Pulsar::Settings::Hook UpdateContext(System::UpdateContextWrapper);

s32 System::OnSceneEnter(Random& random) {
    System* self = System::sInstance;
    self->UpdateContext();
    if(self->IsContext(PULSAR_MODE_OTT)) OTT::AddGhostToVS();
    if(self->IsContext(PULSAR_HAW) && self->IsContext(PULSAR_MODE_KO) && GameScene::GetCurrent()->id == SCENE_ID_RACE && SectionMgr::sInstance->sectionParams->onlineParams.currentRaceNumber > 0) {
        KO::HAWChangeData();
    }
    return random.NextLimited(8);
}
kmCall(0x8051ac40, System::OnSceneEnter);

asmFunc System::GetRaceCount() {
    ASM(
        nofralloc;
    lis r5, sInstance@ha;
    lwz r5, sInstance@l(r5);
    lbz r0, System.netMgr.racesPerGP(r5);
    blr;
        )
}

asmFunc System::GetNonTTGhostPlayersCount() {
    ASM(
        nofralloc;
    lis r12, sInstance@ha;
    lwz r12, sInstance@l(r12);
    lbz r29, System.nonTTGhostPlayersCount(r12);
    blr;
        )
}

//Unlock Everything Without Save (_tZ)
kmWrite32(0x80549974, 0x38600001);

//Skip ESRB page
kmRegionWrite32(0x80604094, 0x4800001c, 'E');

const char System::pulsarString[] = "/Pulsar";
const char System::CommonAssets[] = "/CommonAssets.szs";
const char System::breff[] = "/Effect/Pulsar.breff";
const char System::breft[] = "/Effect/Pulsar.breft";
const char* System::ttModeFolders[] ={ "150", "200", "150F", "200F" };

void FriendSelectPage_joinFriend(Pages::FriendInfo* _this, u32 animDir, float animLength) {
    Pulsar::System::sInstance->netMgr.region = RKNet::Controller::sInstance->friends[_this->selectedFriendIdx].statusData.regionId;
    return _this->EndStateAnimated(animDir, animLength);
}

kmCall(0x805d686c, FriendSelectPage_joinFriend);
kmCall(0x805d6754, FriendSelectPage_joinFriend);

}//namespace Pulsar