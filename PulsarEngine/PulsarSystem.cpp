#include <core/RK/RKSystem.hpp>
#include <core/nw4r/ut/Misc.hpp>
#include <MarioKartWii/Scene/RootScene.hpp>
#include <MarioKartWii/GlobalFunctions.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <PulsarSystem.hpp>
#include <Extensions/LECODE/LECODEMgr.hpp>
#include <Gamemodes/KO/KOMgr.hpp>
#include <Gamemodes/KO/KOHost.hpp>
#include <Gamemodes/OnlineTT/OnlineTT.hpp>
#include <Settings/Settings.hpp>
#include <Config.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <core/egg/DVD/DvdRipper.hpp>
#include <include/c_string.h>

//All Code Credits go to the WTP Team unless otherwise mentioned.

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
    koMgr(nullptr), ottHideNames(false) {
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
    u32 trackBlocking = this->info.GetTrackBlocking();
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
    char path[IOS::ipcMaxPath];
    snprintf(path, IOS::ipcMaxPath, "%s/%s", this->GetModFolder(), "Settings.pul");
    settings->Init(totalTrophyCount, path); //params
    Settings::Mgr::sInstance = settings;
}

void System::UpdateContext() {
    const RacedataSettings& racedataSettings = Racedata::sInstance->menusScenario.settings;
    this->ottVoteState = OTT::COMBO_NONE;
    const Settings::Mgr& settings = Settings::Mgr::Get();
    bool isCT = true;
    bool isHAW = false;
    bool isKO = false;
    bool isOTT = false;
    bool isMiiHeads = settings.GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_MII);

    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const GameMode mode = racedataSettings.gamemode;
    Network::Mgr& netMgr = this->netMgr;
    const u32 sceneId = GameScene::GetCurrent()->id;


    bool is200 = racedataSettings.engineClass == CC_100 && this->info.Has200cc();

    bool is50 = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, HOSTSETTING_CC_50);
    bool is100 = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, HOSTSETTING_CC_REAL100);
    bool is400 = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, HOSTSETTING_CC_400);
    bool is99999 = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, HOSTSETTING_CC_99999);
    bool isRankedFrooms = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGDKW_RANKED_FROOMS) == DKWSETTING_RANKEDFROOMS_ENABLED;

    bool isKOFinal = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, SETTINGKO_FINAL) == KOSETTING_FINAL_ALWAYS;

    bool isOTTChangeCombo = settings.GetSettingValue(Settings::SETTINGSTYPE_OTT, SETTINGOTT_ALLOWCHANGECOMBO) == OTTSETTING_COMBO_ENABLED;

    bool isMayhemCodes = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_MAYHEM_CODES) == DKWSETTING_MAYHEM_ENABLED;
    bool isSnaking = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_SNAKING) == DKWSETTING_SNAKING_ENABLED;
    bool isDisableInvisWalls = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_INVIS_WALLS) == DKWSETTING_INVISWALLS_DISABLED;
    bool isItemBoxSpawnFast = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ITEMBOXRESPAWN) == DKWSETTING_ITEMBOX_FASTRESPAWN;
    bool isItemBoxSpawnInstant = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ITEMBOXRESPAWN) == DKWSETTING_ITEMBOX_INSTANTRESPAWN;
    bool isRiiBalanced = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_RIIBALANCED;
    bool isBumperKart = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_BUMPERKARTS;
    bool isMayhemStats = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_MAYHEM;
    bool isItemModeRandom = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_RANDOM;
    bool isItemModeShellShock = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_SHELLSHOCK;
    bool isItemModeUnknown = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_UNKNOWNITEMS;
    bool isItemModeRain = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_ITEMRAIN;
    bool isItemModeBattleRoyale = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_BATTLEROYALE;
    bool isTrackSelectionRegs = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_TRACKS) == DKWSETTING_TRACKSELECTION_REGS;
    bool isTrackSelectionCts = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_TRACKS) == DKWSETTING_TRACKSELECTION_CTS;
    bool isTrackSelectionSits = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_TRACKS) == DKWSETTING_TRACKSELECTION_SITS && mode != MODE_PUBLIC_VS;

    bool isCharRestrictLight = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_LIGHT;
    bool isCharRestrictMedium = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_MEDIUM;
    bool isCharRestrictHeavy = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_HEAVY;
    bool isKartRestrictKart = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_KARTS;
    bool isKartRestrictBike = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_BIKES;
    bool isTransmissionVanilla = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGHOST_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_VANILLA;
    bool isTransmissionInsideAll = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGHOST_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_INSIDEALL;
    bool isTransmissionOutsideAll = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGHOST_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_OUTSIDEALL;
    bool isCantBrakeDrift = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_BDRIFTING) == DKWSETTING_150_BRAKEDRIFT_OFF;
    bool isCantFastFall = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_FALLFAST) == DKWSETTING_150_FALLFASTOFF;
    bool isCantAlwaysDrift = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_ALWAYSDRIFT) == DKWSETTING_ALLOW_DANYWHEREOFF;
    bool isUltras = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_ULTRAS) == DKWSETTING_ULTRAS_ENABLED;

    bool isThunderCloud = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_THUNDERCLOUD);
    bool isFlyingBlooper = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_FLYINGBLOOP);
    bool isRandomTC = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_THUNDERCLOUD) == DKWSETTING_THUNDERCLOUD_RANDOM;
    bool isTCToggle = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_TCTOGGLE) == DKWSETTING_TCTOGGLE_ENABLED;
    bool isItemStatus = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_ITEMSTATUS) == DKWSETTING_ITEMSTATUS_ENABLED;
    bool isAllItems = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_ALLITEMS) == DKWSETTING_ALLITEMS_ENABLED;
    bool isBulletIcon = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_BULLETICON) == DKWSETTING_BULLETICON_ENABLED;

    bool isFeather = this->info.HasFeather();
    bool isUMTs = this->info.HasUMTs();
    bool isMegaTC = this->info.HasMegaTC();
    u64 newContext = 0;
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
                isKOFinal = newContext & (1ULL << PULSAR_KOFINAL);
                isKO = newContext & (1ULL << PULSAR_MODE_KO);
                isMayhemCodes = newContext & (1ULL << PULSAR_CODES);
                isMayhemStats = newContext & (1ULL << PULSAR_MAYHEMSTATS);
                isRankedFrooms = newContext & (1ULL << PULSAR_RANKED);
                isSnaking = newContext & (1ULL << PULSAR_SNAKING);
                isTCToggle = newContext & (1ULL << PULSAR_TCTOGGLE);
                isItemStatus = newContext & (1ULL << PULSAR_ITEMSTATUS);
                isItemModeBattleRoyale = newContext & (1ULL << PULSAR_BATTLEROYALE);
                isRiiBalanced = newContext & (1ULL << PULSAR_RIIBALANCEDSTATS);
                isBumperKart = newContext & (1ULL << PULSAR_BUMPERKARTSTATS);
                isCharRestrictLight = newContext & (1ULL << PULSAR_CHARRESTRICTLIGHT);
                isCharRestrictMedium = newContext & (1ULL << PULSAR_CHARRESTRICTMEDIUM);
                isCharRestrictHeavy = newContext & (1ULL << PULSAR_CHARRESTRICTHEAVY);
                isKartRestrictKart = newContext & (1ULL << PULSAR_KARTRESTRICT);
                isKartRestrictBike = newContext & (1ULL << PULSAR_BIKERESTRICT);
                isItemModeRandom = newContext & (1ULL << PULSAR_GAMEMODERANDOM);
                isItemModeShellShock = newContext & (1ULL << PULSAR_GAMEMODESHELLSHOCK);
                isItemModeUnknown = newContext & (1ULL << PULSAR_GAMEMODEUNKNOWN);
                isItemModeRain = newContext & (1ULL << PULSAR_GAMEMODEITEMRAIN);
                isUltras = newContext & (1ULL << PULSAR_ULTRAS);
                is50 = newContext & (1ULL << PULSAR_50);
                is100 = newContext & (1ULL << PULSAR_100);
                is400 = newContext & (1ULL << PULSAR_400);
                is99999 = newContext & (1ULL << PULSAR_99999);
                isHAW = newContext & (1ULL << PULSAR_HAW);
                isOTT = newContext & (1ULL << PULSAR_MODE_OTT);
                isMiiHeads = newContext & (1ULL << PULSAR_MIIHEADS);
                isTrackSelectionCts = newContext & (1ULL << PULSAR_CTS);
                isTrackSelectionSits = newContext & (1ULL << PULSAR_SITS);
                isTrackSelectionRegs = newContext & (1ULL << PULSAR_REGS);
                isThunderCloud = newContext & (1ULL << PULSAR_THUNDERCLOUD);
                isFlyingBlooper = newContext & (1ULL << PULSAR_FLYINGBLOOP);
                isAllItems = newContext & (1ULL << PULSAR_ALLITEMS);
                isBulletIcon = newContext & (1ULL << PULSAR_BULLETICON);
                isRandomTC = newContext & (1ULL << PULSAR_RANDOMTC);
                isItemBoxSpawnFast = newContext & (1ULL << PULSAR_ITEMBOXFAST);
                isItemBoxSpawnInstant = newContext & (1ULL << PULSAR_ITEMBOXINSTANT);
                isTransmissionVanilla = newContext & (1ULL << PULSAR_TRANSMISSIONVANILLA);
                isTransmissionInsideAll = newContext & (1ULL << PULSAR_TRANSMISSIONINSIDEALL);
                isTransmissionOutsideAll = newContext & (1ULL << PULSAR_TRANSMISSIONOUTSIDEALL);
                isCantBrakeDrift = newContext & (1ULL << PULSAR_BDRIFTING);
                isCantFastFall = newContext & (1ULL << PULSAR_FALLFAST);
                isCantAlwaysDrift = newContext & (1ULL << PULSAR_NODRIFTANYWHERE);
                isDisableInvisWalls = newContext & (1ULL << PULSAR_INVISWALLS);
                if(isOTT) {
                    isUMTs = newContext & (1ULL << PULSAR_UMTS);
                    isFeather &= newContext & (1ULL << PULSAR_FEATHER);
                    isOTTChangeCombo = newContext & (1ULL << PULSAR_CHANGECOMBO);
                }
                break;
            default: isCT = false;
        }
    }
    else {
        const u8 ottOffline = settings.GetSettingValue(Settings::SETTINGSTYPE_OTT, SETTINGOTT_OFFLINE);
        isOTT = (mode == MODE_GRAND_PRIX || mode == MODE_VS_RACE) ? (ottOffline != OTTSETTING_OFFLINE_DISABLED) : false; //offlineOTT
        if(isOTT) {
            isFeather &= (ottOffline == OTTSETTING_OFFLINE_FEATHER);
            isUltras &= ~settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ALLOW_ULTRAS);
        }
    }
    this->netMgr.hostContext = newContext;

    u64 context = ((u64)isCT << PULSAR_CT) | ((u64)isHAW << PULSAR_HAW) | ((u64)isMiiHeads << PULSAR_MIIHEADS);
    if(isCT) { //contexts that should only exist when CTs are on
        context |= ((u64)is200 << PULSAR_200) | ((u64)is50 << PULSAR_50) | ((u64)is100 << PULSAR_100) | ((u64)is400 << PULSAR_400) |((u64)is99999 << PULSAR_99999) | ((u64)isFeather << PULSAR_FEATHER) | ((u64)isUMTs << PULSAR_UMTS) | ((u64)isMegaTC << PULSAR_MEGATC) | ((u64)isOTT << PULSAR_MODE_OTT) | ((u64)isKO << PULSAR_MODE_KO) | ((u64)isUltras << PULSAR_ULTRAS) | ((u64)isTrackSelectionCts << PULSAR_CTS) | ((u64)isTrackSelectionSits << PULSAR_SITS) | ((u64)isTrackSelectionRegs << PULSAR_REGS) | ((u64)isCharRestrictLight << PULSAR_CHARRESTRICTLIGHT) | ((u64)isCharRestrictMedium << PULSAR_CHARRESTRICTMEDIUM) | ((u64)isCharRestrictHeavy << PULSAR_CHARRESTRICTHEAVY) | ((u64)isKartRestrictKart << PULSAR_KARTRESTRICT) | ((u64)isKartRestrictBike << PULSAR_BIKERESTRICT) | ((u64)isItemModeRandom << PULSAR_GAMEMODERANDOM) | ((u64)isBumperKart << PULSAR_BUMPERKARTSTATS) | ((u64)isRiiBalanced << PULSAR_RIIBALANCEDSTATS) | ((u64)isItemModeShellShock << PULSAR_GAMEMODESHELLSHOCK) | ((u64)isItemModeUnknown << PULSAR_GAMEMODEUNKNOWN) | ((u64)isItemModeRain << PULSAR_GAMEMODEITEMRAIN) | ((u64)isKOFinal << PULSAR_KOFINAL) | ((u64)isOTTChangeCombo << PULSAR_CHANGECOMBO) | ((u64)isThunderCloud << PULSAR_THUNDERCLOUD) | ((u64)isFlyingBlooper << PULSAR_FLYINGBLOOP) | ((u64)isRandomTC << PULSAR_RANDOMTC) | ((u64)isTransmissionVanilla << PULSAR_TRANSMISSIONVANILLA) | ((u64)isTransmissionInsideAll << PULSAR_TRANSMISSIONINSIDEALL) | ((u64)isTransmissionOutsideAll << PULSAR_TRANSMISSIONOUTSIDEALL) | ((u64)isCantBrakeDrift << PULSAR_BDRIFTING) | ((u64)isCantFastFall << PULSAR_FALLFAST) | ((u64)isCantAlwaysDrift << PULSAR_NODRIFTANYWHERE) | ((u64)isDisableInvisWalls << PULSAR_INVISWALLS) | ((u64)isAllItems << PULSAR_ALLITEMS) | ((u64)isBulletIcon << PULSAR_BULLETICON);
    }
    if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) { //contexts that should only exist when you're in a froom
        context |= ((u64)isMayhemStats << PULSAR_MAYHEMSTATS) | ((u64)isMayhemCodes << PULSAR_CODES) | ((u64)isRankedFrooms << PULSAR_RANKED) | ((u64)isSnaking << PULSAR_SNAKING) | ((u64)isTCToggle << PULSAR_TCTOGGLE) | ((u64)isItemStatus << PULSAR_ITEMSTATUS) | ((u64)isItemBoxSpawnFast << PULSAR_ITEMBOXFAST) | ((u64)isItemBoxSpawnInstant << PULSAR_ITEMBOXINSTANT) | ((u64)isItemModeBattleRoyale << PULSAR_BATTLEROYALE);
    }
    this->context = context;

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

}//namespace Pulsar