#include <kamek.hpp>
#include <MarioKartWii/RKNet/ROOM.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <Settings/UI/SettingsPanel.hpp>
#include <Settings/Settings.hpp>
#include <Network/Network.hpp>
#include <Network/PacketExpansion.hpp>

namespace Pulsar {
namespace Network {

//Implements the ability for a host to send a message, allowing for custom host settings

//If we are in a room, we are guaranteed to be in a situation where Pul packets are being sent
//however, no reason to send the settings outside of START packets and if we are not the host, this is easily changed by just editing the check

static void ConvertROOMPacketToData(const PulROOM& packet) {
    System* system = System::sInstance;
    system->netMgr.hostContext = packet.hostSystemContext;
    system->netMgr.racesPerGP = packet.raceCount;
    system->netMgr.KOContext = packet.KOSystemContext;
}

static void BeforeROOMSend(RKNet::PacketHolder<PulROOM>* packetHolder, PulROOM* src, u32 len) {
    packetHolder->Copy(src, len); //default

    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];
    Pulsar::System* system = Pulsar::System::sInstance;
    PulROOM* destPacket = packetHolder->packet;
    if (destPacket->messageType == 1 && sub.localAid == sub.hostAid) {
        packetHolder->packetSize = sizeof(PulROOM); //this has been changed by copy so it's safe to do this
        const Settings::Mgr& settings = Settings::Mgr::Get();
        const RacedataSettings& racedataSettings = Racedata::sInstance->menusScenario.settings;
        const GameMode mode = racedataSettings.gamemode;

        u8 koSetting = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, SETTINGKO_ENABLED) && destPacket->message == 0;
        u8 ottOnline = settings.GetSettingValue(Settings::SETTINGSTYPE_OTT, SETTINGOTT_ONLINE);
        const u8 ottChangeCombo = settings.GetSettingValue(Settings::SETTINGSTYPE_OTT, SETTINGOTT_ALLOWCHANGECOMBO) == OTTSETTING_COMBO_ENABLED;
        const u8 koFinal = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, SETTINGKO_FINAL) == KOSETTING_FINAL_ALWAYS;
        const u8 fiftyCC = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_RADIO_CC) == HOSTSETTING_CC_50;
        const u8 hundredCC = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_RADIO_CC) == HOSTSETTING_CC_REAL100;
        const u8 fourCC = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_RADIO_CC) == HOSTSETTING_CC_400;
        const u8 ninetyCC = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_RADIO_CC) == HOSTSETTING_CC_99999;
        const u8 rankedFrooms = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGDKW_RANKED_FROOMS) == DKWSETTING_RANKEDFROOMS_ENABLED;

        const u8 mayhemCodes = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_MAYHEM_CODES) == DKWSETTING_MAYHEM_ENABLED;
        const u8 snaking = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_SNAKING) == DKWSETTING_SNAKING_ENABLED;
        const u8 itemModeRandom = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_RANDOM;
        const u8 itemModeShellShock = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_SHELLSHOCK;
        const u8 itemModeUnknown = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_UNKNOWNITEMS;
        const u8 itemModeRain = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_ITEMRAIN;
        const u8 itemModeBattleRoyale = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_BATTLEROYALE;
        const u8 RegOnly = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_TRACKS) == DKWSETTING_TRACKSELECTION_REGS;
        const u8 SitsOnly = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_TRACKS) == DKWSETTING_TRACKSELECTION_SITS && mode != MODE_PUBLIC_VS;
        const u8 CtsOnly = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_TRACKS) == DKWSETTING_TRACKSELECTION_CTS;
        const u8 bumperKart = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_BUMPERKARTS;
        const u8 riiBalanced = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_RIIBALANCED;
        const u8 mayhemStats = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_MAYHEM;
        const u8 itemBoxSpawnFast = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ITEMBOXRESPAWN) == DKWSETTING_ITEMBOX_FASTRESPAWN;
        const u8 itemBoxSpawnInstant = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ITEMBOXRESPAWN) == DKWSETTING_ITEMBOX_INSTANTRESPAWN;
        const u8 disableInvisWalls = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_INVIS_WALLS) == DKWSETTING_INVISWALLS_DISABLED;

        const u8 ultras = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_ULTRAS) == DKWSETTING_ULTRAS_ENABLED;
        const u8 charRestrictLight = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_LIGHT;
        const u8 charRestrictMedium = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_MEDIUM;
        const u8 charRestrictHeavy = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_HEAVY;
        const u8 charRestrictPrincess = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_PRINCESS;
        const u8 kartRestrict = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_KARTS;
        const u8 bikeRestrict = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_BIKES;
        const u8 transmissionVanilla = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGHOST_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_VANILLA;
        const u8 transmissionInsideAll = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGHOST_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_INSIDEALL;
        const u8 transmissionOutsideAll = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGHOST_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_OUTSIDEALL;
        const u8 cantBrakeDrift = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_BDRIFTING) == DKWSETTING_150_BRAKEDRIFT_OFF;
        const u8 cantFastFall = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_FALLFAST) == DKWSETTING_150_FALLFASTOFF;
        const u8 cantAlwaysDrift = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_ALWAYSDRIFT) == DKWSETTING_ALLOW_DANYWHEREOFF;

        const u8 randomTC = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_THUNDERCLOUD) == DKWSETTING_THUNDERCLOUD_RANDOM;
        const u8 tcToggle = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_TCTOGGLE) == DKWSETTING_TCTOGGLE_ENABLED;
        const u8 itemStatus = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_ITEMSTATUS) == DKWSETTING_ITEMSTATUS_ENABLED;
        const u8 allItems = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_ALLITEMS) == DKWSETTING_ALLITEMS_ENABLED;
        const u8 bulletIcon = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_BULLETICON) == DKWSETTING_BULLETICON_ENABLED;

        if (rankedFrooms) {
            koSetting = KOSETTING_DISABLED;
            ottOnline = OTTSETTING_ONLINE_DISABLED;
        }

        destPacket->hostSystemContext = (static_cast<u64>(ottOnline != OTTSETTING_OFFLINE_DISABLED) << PULSAR_MODE_OTT)
      | (static_cast<u64>(koSetting) << PULSAR_MODE_KO)
      | (static_cast<u64>(koFinal) << PULSAR_KOFINAL)
      | (static_cast<u64>(ottOnline == OTTSETTING_ONLINE_FEATHER) << PULSAR_FEATHER) 
      | (static_cast<u64>(settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ALLOW_ULTRAS) ^ true) << PULSAR_ULTRAS)
      | (static_cast<u64>(ottChangeCombo) << PULSAR_CHANGECOMBO)
      | (static_cast<u64>(settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_ALLOW_MIIHEADS) ^ true) << PULSAR_MIIHEADS)
      | (static_cast<u64>(itemModeRain) << PULSAR_GAMEMODEITEMRAIN)
      | (static_cast<u64>(mayhemCodes) << PULSAR_CODES)
      | (static_cast<u64>(mayhemStats) << PULSAR_MAYHEMSTATS)
      | (static_cast<u64>(rankedFrooms) << PULSAR_RANKED)
      | (static_cast<u64>(riiBalanced) << PULSAR_RIIBALANCEDSTATS)
      | (static_cast<u64>(bumperKart) << PULSAR_BUMPERKARTSTATS)
      | (static_cast<u64>(charRestrictLight) << PULSAR_CHARRESTRICTLIGHT)
      | (static_cast<u64>(charRestrictMedium) << PULSAR_CHARRESTRICTMEDIUM)
      | (static_cast<u64>(charRestrictHeavy) << PULSAR_CHARRESTRICTHEAVY)
      | (static_cast<u64>(charRestrictPrincess) << PULSAR_CHARRESTRICTPRINCESS)
      | (static_cast<u64>(kartRestrict) << PULSAR_KARTRESTRICT)
      | (static_cast<u64>(bikeRestrict) << PULSAR_BIKERESTRICT)
      | (static_cast<u64>(itemModeRandom) << PULSAR_GAMEMODERANDOM)
      | (static_cast<u64>(itemModeShellShock) << PULSAR_GAMEMODESHELLSHOCK)
      | (static_cast<u64>(itemModeUnknown) << PULSAR_GAMEMODEUNKNOWN)
      | (static_cast<u64>(itemModeBattleRoyale) << PULSAR_BATTLEROYALE)
      | (static_cast<u64>(itemBoxSpawnFast) << PULSAR_ITEMBOXFAST)
      | (static_cast<u64>(itemBoxSpawnInstant) << PULSAR_ITEMBOXINSTANT)
      | (static_cast<u64>(tcToggle) << PULSAR_TCTOGGLE)
      | (static_cast<u64>(itemStatus) << PULSAR_ITEMSTATUS)
      | (static_cast<u64>(allItems) << PULSAR_ALLITEMS)
      | (static_cast<u64>(bulletIcon) << PULSAR_BULLETICON)
      | (static_cast<u64>(snaking) << PULSAR_SNAKING)
      | (static_cast<u64>(RegOnly) << PULSAR_REGS)
      | (static_cast<u64>(SitsOnly) << PULSAR_SITS)
      | (static_cast<u64>(CtsOnly) << PULSAR_CTS)
      | (static_cast<u64>(transmissionVanilla) << PULSAR_TRANSMISSIONVANILLA)
      | (static_cast<u64>(transmissionInsideAll) << PULSAR_TRANSMISSIONINSIDEALL)
      | (static_cast<u64>(transmissionOutsideAll) << PULSAR_TRANSMISSIONOUTSIDEALL)
      | (static_cast<u64>(randomTC) << PULSAR_RANDOMTC)
      | (static_cast<u64>(cantBrakeDrift) << PULSAR_BDRIFTING)
      | (static_cast<u64>(cantFastFall) << PULSAR_FALLFAST)
      | (static_cast<u64>(cantAlwaysDrift) << PULSAR_NODRIFTANYWHERE)
      | (static_cast<u64>(disableInvisWalls) << PULSAR_INVISWALLS)
      | (static_cast<u64>(fiftyCC) << PULSAR_50)
      | (static_cast<u64>(hundredCC) << PULSAR_100)
      | (static_cast<u64>(fourCC) << PULSAR_400)
      | (static_cast<u64>(ninetyCC) << PULSAR_99999)
      | (static_cast<u64>(settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_THUNDERCLOUD)) << PULSAR_THUNDERCLOUD)
      | (static_cast<u64>(settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_FLYINGBLOOP)) << PULSAR_FLYINGBLOOP)
      | (static_cast<u64>(settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_RADIO_HOSTWINS)) << PULSAR_HAW);

      destPacket->KOSystemContext = (static_cast<u64>(koSetting) << PULSAR_MODE_KO)
      | (static_cast<u64>(koFinal) << PULSAR_KOFINAL);
  
        u8 raceCount;
        if (koSetting == KOSETTING_ENABLED) raceCount = 0xFE;
        else switch (settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_SCROLL_GP_RACES)) {
        case(1):
            raceCount = 7;
            break;
        case(2):
            raceCount = 11;
            break;
        case(3):
            raceCount = 23;
            break;
        case(4):
            raceCount = 31;
            break;
        case(5):
            raceCount = 63;
            break;
        case(6):
            raceCount = 1;
            break;
        default:
            raceCount = 3;
        }
        destPacket->raceCount = raceCount;
        ConvertROOMPacketToData(*destPacket);
    }
}
kmCall(0x8065b15c, BeforeROOMSend);

kmWrite32(0x8065add0, 0x60000000);
static void AfterROOMReception(const RKNet::PacketHolder<PulROOM>* packetHolder, const PulROOM& src, u32 len) {
    register RKNet::ROOMPacket* packet;
    register u32 aid;
    asm(mr packet, r28;);
    asm(mr aid, r29;);
    
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];
    
    const bool isHost = sub.localAid == sub.hostAid;

    //START msg sent by the host, size check should always be guaranteed in theory
    if(src.messageType == 1 && !isHost && packetHolder->packetSize == sizeof(PulROOM)) {
        ConvertROOMPacketToData(src);
        const Settings::Mgr& settings = Settings::Mgr::Get();

    bool is50 = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, HOSTSETTING_CC_50);
    bool is100 = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, HOSTSETTING_CC_REAL100);
    bool is400 = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, HOSTSETTING_CC_400);
    bool is99999 = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, HOSTSETTING_CC_99999);

    bool isCharRestrictLight = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_LIGHT;
    bool isCharRestrictMedium = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_MEDIUM;
    bool isCharRestrictHeavy = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_HEAVY;
    bool isCharRestrictPrincess = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_PRINCESS;
    bool isKartRestrictKart = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_KARTS;
    bool isKartRestrictBike = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_BIKES;
    bool isItemModeRandom = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_RANDOM;
    bool isMayhemStats = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_MAYHEM;
    bool isRiiBalanced = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_RIIBALANCED;
    bool isBumperKart = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_VEHICLESTATS) == DKWSETTING_STATS_BUMPERKARTS;
    bool isItemModeShellShock = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_SHELLSHOCK;
    bool isItemModeUnknown = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_UNKNOWNITEMS;
    bool isItemModeRain = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_ITEMRAIN;
    bool isItemModeBattleRoyale = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_GAMEMODE) == DKWSETTING_GAMEMODE_BATTLEROYALE;
    bool isRandomTC = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_THUNDERCLOUD) == DKWSETTING_THUNDERCLOUD_RANDOM;
    bool isCantBrakeDrift = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_BDRIFTING) == DKWSETTING_150_BRAKEDRIFT_OFF;
    bool isCantFastFall = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_FALLFAST) == DKWSETTING_150_FALLFASTOFF;
    bool isCantAlwaysDrift = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW3, SETTINGDKW_ALLOW_ALWAYSDRIFT) == DKWSETTING_ALLOW_DANYWHEREOFF;
    bool isDisableInvisWalls = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_INVIS_WALLS) == DKWSETTING_INVISWALLS_DISABLED;

    bool isRankedFrooms = settings.GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGDKW_RANKED_FROOMS) == DKWSETTING_RANKEDFROOMS_ENABLED;
    bool isMayhemCodes = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_MAYHEM_CODES) == DKWSETTING_MAYHEM_ENABLED;
    bool isSnaking = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_SNAKING) == DKWSETTING_SNAKING_ENABLED;
    bool isItemBoxSpawnFast = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ITEMBOXRESPAWN) == DKWSETTING_ITEMBOX_FASTRESPAWN;
    bool isItemBoxSpawnInstant = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW2, SETTINGDKW_ITEMBOXRESPAWN) == DKWSETTING_ITEMBOX_INSTANTRESPAWN;
    bool isTcToggle = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_TCTOGGLE) == DKWSETTING_TCTOGGLE_ENABLED;
    bool isItemStatus = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_ITEMSTATUS) == DKWSETTING_ITEMSTATUS_ENABLED;
    bool isAllItems = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_ALLITEMS) == DKWSETTING_ALLITEMS_ENABLED;
    bool isBulletIcon = settings.GetUserSettingValue(Settings::SETTINGSTYPE_DKW4, SETTINGDKW_BULLETICON) == DKWSETTING_BULLETICON_ENABLED;
        
    u64 newContext = 0;
    Network::Mgr& netMgr = Pulsar::System::sInstance->netMgr;
        newContext = netMgr.hostContext;
        is50 = newContext & (1ULL << PULSAR_50);
        is100 = newContext & (1ULL << PULSAR_100);
        is400 = newContext & (1ULL << PULSAR_400);
        is99999 = newContext & (1ULL << PULSAR_99999);
        isCharRestrictLight = newContext & (1ULL << PULSAR_CHARRESTRICTLIGHT);
        isCharRestrictMedium = newContext & (1ULL << PULSAR_CHARRESTRICTMEDIUM);
        isCharRestrictHeavy = newContext & (1ULL << PULSAR_CHARRESTRICTHEAVY);
        isCharRestrictPrincess = newContext & (1ULL << PULSAR_CHARRESTRICTPRINCESS);
        isKartRestrictKart = newContext & (1ULL << PULSAR_KARTRESTRICT);
        isKartRestrictBike = newContext & (1ULL << PULSAR_BIKERESTRICT);
        isMayhemStats = newContext & (1 << PULSAR_MAYHEMSTATS);
        isRiiBalanced = newContext & (1 << PULSAR_RIIBALANCEDSTATS);
        isBumperKart = newContext & (1 << PULSAR_BUMPERKARTSTATS);
        isItemModeRandom = newContext & (1 << PULSAR_GAMEMODERANDOM);
        isItemModeShellShock = newContext & (1 << PULSAR_GAMEMODESHELLSHOCK);
        isItemModeUnknown = newContext & (1 << PULSAR_GAMEMODEUNKNOWN);
        isItemModeRain = newContext & (1 << PULSAR_GAMEMODEITEMRAIN);
        isItemModeBattleRoyale = newContext & (1ULL << PULSAR_BATTLEROYALE);
        isRandomTC = newContext & (1ULL << PULSAR_RANDOMTC);
        isCantBrakeDrift = newContext & (1ULL << PULSAR_BDRIFTING);
        isCantFastFall = newContext & (1ULL << PULSAR_FALLFAST);
        isCantAlwaysDrift = newContext & (1ULL << PULSAR_NODRIFTANYWHERE);
        isDisableInvisWalls = newContext & (1ULL << PULSAR_INVISWALLS);
        isBulletIcon = newContext & (1ULL << PULSAR_BULLETICON);
        isRankedFrooms = newContext & (1ULL << PULSAR_RANKED);
        isMayhemCodes = newContext & (1ULL << PULSAR_CODES);
        isSnaking = newContext & (1ULL << PULSAR_SNAKING);
        isItemBoxSpawnFast = newContext & (1ULL << PULSAR_ITEMBOXFAST);
        isItemBoxSpawnInstant = newContext & (1ULL << PULSAR_ITEMBOXINSTANT);
        isTcToggle = newContext & (1ULL << PULSAR_TCTOGGLE);
        isItemStatus = newContext & (1ULL << PULSAR_ITEMSTATUS);
        isAllItems = newContext & (1ULL << PULSAR_ALLITEMS);
    netMgr.hostContext = newContext;

    

    u64 context = (static_cast<u64>(is50) << PULSAR_50) | (static_cast<u64>(is100) << PULSAR_100) | (static_cast<u64>(is400) << PULSAR_400) | (static_cast<u64>(is99999) << PULSAR_99999) | (static_cast<u64>(isItemModeBattleRoyale) << PULSAR_BATTLEROYALE) | (static_cast<u64>(isItemModeRain) << PULSAR_GAMEMODEITEMRAIN) | (static_cast<u64>(isCharRestrictLight) << PULSAR_CHARRESTRICTLIGHT) | (static_cast<u64>(isCharRestrictMedium) << PULSAR_CHARRESTRICTMEDIUM) | (static_cast<u64>(isCharRestrictHeavy) << PULSAR_CHARRESTRICTHEAVY) | (static_cast<u64>(isCharRestrictPrincess) << PULSAR_CHARRESTRICTPRINCESS) | (static_cast<u64>(isKartRestrictKart) << PULSAR_KARTRESTRICT) | (static_cast<u64>(isKartRestrictBike) << PULSAR_BIKERESTRICT) | (static_cast<u64>(isMayhemStats) << PULSAR_MAYHEMSTATS) | (static_cast<u64>(isRiiBalanced) << PULSAR_RIIBALANCEDSTATS) | (static_cast<u64>(isBumperKart) << PULSAR_BUMPERKARTSTATS) | (static_cast<u64>(isItemModeRandom) << PULSAR_GAMEMODERANDOM) | (static_cast<u64>(isItemModeShellShock) << PULSAR_GAMEMODESHELLSHOCK) | (static_cast<u64>(isItemModeUnknown) << PULSAR_GAMEMODEUNKNOWN) | (static_cast<u64>(isRandomTC) << PULSAR_RANDOMTC) | (static_cast<u64>(isCantBrakeDrift) << PULSAR_BDRIFTING) | (static_cast<u64>(isCantFastFall) << PULSAR_FALLFAST) | (static_cast<u64>(isCantAlwaysDrift) << PULSAR_NODRIFTANYWHERE) | (static_cast<u64>(isDisableInvisWalls) << PULSAR_INVISWALLS) | (static_cast<u64>(isRankedFrooms) << PULSAR_RANKED) | (static_cast<u64>(isMayhemCodes) << PULSAR_CODES) | (static_cast<u64>(isSnaking) << PULSAR_SNAKING) | (static_cast<u64>(isItemBoxSpawnFast) << PULSAR_ITEMBOXFAST) | (static_cast<u64>(isItemBoxSpawnInstant) << PULSAR_ITEMBOXINSTANT) | (static_cast<u64>(isTcToggle) << PULSAR_TCTOGGLE) | (static_cast<u64>(isItemStatus) << PULSAR_ITEMSTATUS) | (static_cast<u64>(isAllItems) << PULSAR_ALLITEMS) | (static_cast<u64>(isBulletIcon) << PULSAR_BULLETICON);
    Pulsar::System::sInstance->context = context;
        
        //Also exit the settings page to prevent weird graphical artefacts
        Page* topPage = SectionMgr::sInstance->curSection->GetTopLayerPage();
        PageId topId = topPage->pageId;
        if (topId == UI::SettingsPanel::id) {
            UI::SettingsPanel* panel = static_cast<UI::SettingsPanel*>(topPage);
            panel->OnBackPress(0);
        }
    }
    memcpy(packet, &src, sizeof(RKNet::ROOMPacket)); //default
}
kmCall(0x8065add8, AfterROOMReception);

/*
//ROOMPacket bits arrangement: 0-4 GPraces
//u8 racesPerGP = 0;



//Adds the settings to the free bits of the packet, only called for the host, msgType1 has 14 free bits as the game only has 4 gamemodes
void SetAllToSendPackets(RKNet::ROOMHandler& roomHandler, u32 packetArg) {
    RKNet::ROOMPacketReg packetReg ={ packetArg };
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const u8 localAid = controller->subs[controller->currentSub].localAid;
    Pulsar::System* system = Pulsar::System::sInstance;
    if((packetReg.packet.messageType) == 1 && localAid == controller->subs[controller->currentSub].hostAid) {
        const u8 hostParam = Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_RADIO_HOSTWINS);
        packetReg.packet.message |= hostParam << 2; //uses bit 2 of message

        const u8 gpParam = Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_SCROLL_GP_RACES);
        const u8 disableMiiHeads = Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_HOST, SETTINGHOST_ALLOW_MIIHEADS);
        packetReg.packet.message |= gpParam << 3; //uses bits 3-5
        packetReg.packet.message |= disableMiiHeads << 6; //uses bit 6
        packetReg.packet.message |= Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_OTT, SETTINGOTT_ONLINE) << 7; //7 for OTT
        packetReg.packet.message |= Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_KO, SETTINGKO_ENABLED) << 8; //8 for KO

        ConvertROOMPacketToData(packetReg.packet.message >> 2); //5 right now (2-8) + 1 reserved (9)
        packetReg.packet.message |= (System::sInstance->SetPackROOMMsg() << 0xA & 0b1111110000000000); //6 bits for packs (10-15)
    }
    for(int i = 0; i < 12; ++i) if(i != localAid) roomHandler.toSendPackets[i] = packetReg.packet;
}
kmBranch(0x8065ae70, SetAllToSendPackets);
//kmCall(0x805dce34, SetAllToSendPackets);
//kmCall(0x805dcd2c, SetAllToSendPackets);
//kmCall(0x805d9fe8, SetAllToSendPackets);

//Non-hosts extract the setting, store it and then return the packet without these bits
RKNet::ROOMPacket GetParamFromPacket(u32 packetArg, u8 aidOfSender) {
    RKNet::ROOMPacketReg packetReg ={ packetArg };
    if(packetReg.packet.messageType == 1) {
        const RKNet::Controller* controller = RKNet::Controller::sInstance;
        //Seeky's code to prevent guests from start the GP
        if(controller->subs[controller->currentSub].hostAid != aidOfSender) packetReg.packet.messageType = 0;
        else {
            ConvertROOMPacketToData((packetReg.packet.message & 0b0000001111111100) >> 2);
            System::sInstance->ParsePackROOMMsg(packetReg.packet.message >> 0xA);
        }
        packetReg.packet.message &= 0x3;
        Page* topPage = SectionMgr::sInstance->curSection->GetTopLayerPage();
        PageId topId = topPage->pageId;
        if(topId == UI::SettingsPanel::id) {
            UI::SettingsPanel* panel = static_cast<UI::SettingsPanel*>(topPage);
            panel->OnBackPress(0);
        }
    }
    return packetReg.packet;
}
kmBranch(0x8065af70, GetParamFromPacket);
*/

//Implements that setting
kmCall(0x806460B8, System::GetRaceCount);
kmCall(0x8064f51c, System::GetRaceCount);
}//namespace Network
}//namespace Pulsar