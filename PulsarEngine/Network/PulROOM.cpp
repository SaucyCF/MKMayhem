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
    system->netMgr.hostContext2 = packet.hostSystemContext2;
    system->netMgr.racesPerGP = packet.raceCount;
}

static bool ApplyHostContextLocally(u32 hostContext) {
    System* system = System::sInstance;
    
    const bool isCharRestrictLight = hostContext & (1 << PULSAR_CHARRESTRICTLIGHT);
    const bool isCharRestrictMedium = hostContext & (1 << PULSAR_CHARRESTRICTMEDIUM);
    const bool isCharRestrictHeavy = hostContext & (1 << PULSAR_CHARRESTRICTHEAVY);
    const bool isKartRestrictKart = hostContext & (1 << PULSAR_KARTRESTRICT);
    const bool isKartRestrictBike = hostContext & (1 << PULSAR_BIKERESTRICT);
    const bool isStartMKDS = hostContext & (1 << PULSAR_STARTMKDS);
    const bool isStartItemRain = hostContext & (1 << PULSAR_STARTITEMRAIN);
    const bool isStartMayhem = hostContext & (1 << PULSAR_STARTMAYHEM);

    u32 context = (isCharRestrictLight << PULSAR_CHARRESTRICTLIGHT) | (isCharRestrictMedium << PULSAR_CHARRESTRICTMEDIUM) | (isCharRestrictHeavy << PULSAR_CHARRESTRICTHEAVY) | (isKartRestrictKart << PULSAR_KARTRESTRICT) | (isKartRestrictBike << PULSAR_BIKERESTRICT) | (isStartMKDS << PULSAR_STARTMKDS) | (isStartItemRain << PULSAR_STARTITEMRAIN) | (isStartMayhem << PULSAR_STARTMAYHEM);
    Pulsar::System::sInstance->context = context;

    if (isStartMKDS || isStartItemRain || isStartMayhem) {
            Pulsar::System::sInstance->context &= ~(1 << PULSAR_CHARRESTRICTHEAVY);
            Pulsar::System::sInstance->context &= ~(1 << PULSAR_CHARRESTRICTMEDIUM);
            Pulsar::System::sInstance->context &= ~(1 << PULSAR_CHARRESTRICTLIGHT);
            Pulsar::System::sInstance->context &= ~(1 << PULSAR_KARTRESTRICT);
            Pulsar::System::sInstance->context &= ~(1 << PULSAR_BIKERESTRICT);
    }
}

static void BeforeROOMSend(RKNet::PacketHolder<PulROOM>* packetHolder, PulROOM* src, u32 len) {
    packetHolder->Copy(src, len); //default

    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];
    Pulsar::System* system = Pulsar::System::sInstance;
    PulROOM* destPacket = packetHolder->packet;
    if (destPacket->messageType == 1 && sub.localAid == sub.hostAid) {
        packetHolder->packetSize = sizeof(PulROOM); //this has been changed by copy so it's safe to do this

        // Store original message index for worldwide option detection
        const u8 originalMessage = destPacket->message;
        if (originalMessage >= 4 && originalMessage <= 6) {
            destPacket->message = 0;
        }

        const Settings::Mgr& settings = Settings::Mgr::Get();
        const RacedataSettings& racedataSettings = Racedata::sInstance->menusScenario.settings;
        const GameMode mode = racedataSettings.gamemode;

        bool isFroom = controller->roomType == RKNet::ROOMTYPE_FROOM_HOST || controller->roomType == RKNet::ROOMTYPE_FROOM_NONHOST;
        bool isFroomStart = destPacket->message == 0;
        bool isBattle = destPacket->message == 2 || destPacket->message == 3;
        bool isBalloonBattle = destPacket->message == 2;
        bool isNotPublic = isFroom || controller->roomType == RKNet::ROOMTYPE_NONE;
        bool isTimeTrial = mode == MODE_TIME_TRIAL;

        u8 koSetting = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, KO_ENABLED) == KOSETTING_ENABLED;
        u8 lapKoSetting = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, KO_ENABLED) == KOSETTING_LAP_ENABLED && isNotPublic && !isBattle && !isTimeTrial;
        const u8 koFinal = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, KO_FINAL) == KOSETTING_FINAL_ALWAYS;

        const u8 worldwideMKDS = settings.GetSettingValue(Settings::SETTINGSTYPE_MISC2, WW_GAMEMODE) == DKWSETTING_WWGAMEMODE_MKDS;
        const u8 worldwideItemRain = settings.GetSettingValue(Settings::SETTINGSTYPE_MISC2, WW_GAMEMODE) == DKWSETTING_WWGAMEMODE_ITEMRAIN;
        const u8 worldwideMayhem = settings.GetSettingValue(Settings::SETTINGSTYPE_MISC2, WW_GAMEMODE) == DKWSETTING_WWGAMEMODE_MAYHEM;
        const u8 startMKDS = (originalMessage == 4);
        const u8 startItemRain = (originalMessage == 5);
        const u8 startMayhem = (originalMessage == 6);

        const u8 fiftyCC = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_CC) == HOSTSETTING_CC_50;
        const u8 hundredCC = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_CC) == HOSTSETTING_CC_REAL100;
        const u8 fourCC = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_CC) == HOSTSETTING_CC_400;
        const u8 ninetyCC = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_CC) == HOSTSETTING_CC_99999;
        const u8 ultras = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_ULTRAS) == DKWSETTING_ULTRAS_ENABLED;
        const u8 cantBrakeDrift = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_BDRIFTING) == DKWSETTING_150_BRAKEDRIFT_OFF;
        const u8 cantFastFall = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_FALLFAST) == DKWSETTING_150_FALLFASTOFF;
        const u8 cantAlwaysDrift = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_ALWAYSDRIFT) == DKWSETTING_ALLOW_DANYWHEREOFF;

        u8 charRestrictLight = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_LIGHT;
        u8 charRestrictMedium = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_MEDIUM;
        u8 charRestrictHeavy = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_CHARRESTRICT) == DKWSETTING_CHARRESTRICT_HEAVY;
        u8 kartRestrict = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_KARTS;
        u8 bikeRestrict = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_VEHICLERESTRICT) == DKWSETTING_VEHICLERESTRICT_BIKES;
        const u8 mayhemCodes = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_MAYHEM_CODES) == DKWSETTING_MAYHEM_ENABLED;
        const u8 itemModeUnknown = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_UNKNOWNITEMS;
        const u8 itemModeRain = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_ITEMRAIN;
        const u8 itemModeBattleRoyale = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_BATTLEROYALE;
        const u8 bumperKart = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_BUMPERKARTS;
        const u8 riiBalanced = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_RIIBALANCED;
        const u8 disableInvisWalls = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_INVIS_WALLS) == DKWSETTING_INVISWALLS_DISABLED;
        const u8 transmissionVanilla = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_VANILLA;
        const u8 transmissionInsideAll = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_INSIDEALL;
        const u8 transmissionOutsideAll = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_OUTSIDEALL;

        const u8 boxSpawnFast = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_FASTSPAWN;
        const u8 boxSpawnInstant = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_INSTANTSPAWN;
        const u8 boxSpawnDisabled = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_DISABLED;
        const u8 tcToggle = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_TCTOGGLE) == DKWSETTING_TCTOGGLE_ENABLED;
        const u8 allItems = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_ALLITEMS) == DKWSETTING_ALLITEMS_ENABLED;

        destPacket->hostSystemContext = (koSetting << PULSAR_MODE_KO)
      | (lapKoSetting) << PULSAR_MODE_LAPKO
      | (koFinal) << PULSAR_KOFINAL
      | (settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_ULTRAS) ^ true) << PULSAR_ULTRAS
      | (settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_MIIHEADS) ^ true) << PULSAR_MIIHEADS
      | (mayhemCodes) << PULSAR_MAYHEM
      | (charRestrictLight) << PULSAR_CHARRESTRICTLIGHT
      | (charRestrictMedium) << PULSAR_CHARRESTRICTMEDIUM
      | (charRestrictHeavy) << PULSAR_CHARRESTRICTHEAVY
      | (kartRestrict) << PULSAR_KARTRESTRICT
      | (bikeRestrict) << PULSAR_BIKERESTRICT
      | (tcToggle) << PULSAR_TCTOGGLE
      | (allItems) << PULSAR_ALLITEMS
      | (transmissionVanilla) << PULSAR_TRANSMISSIONVANILLA
      | (transmissionInsideAll) << PULSAR_TRANSMISSIONINSIDEALL
      | (transmissionOutsideAll) << PULSAR_TRANSMISSIONOUTSIDEALL
      | (cantBrakeDrift) << PULSAR_BDRIFTING
      | (cantFastFall) << PULSAR_FALLFAST
      | (cantAlwaysDrift) << PULSAR_NODRIFTANYWHERE
      | (disableInvisWalls) << PULSAR_INVISWALLS
      | (startMKDS) << PULSAR_STARTMKDS
      | (startItemRain) << PULSAR_STARTITEMRAIN
      | (startMayhem) << PULSAR_STARTMAYHEM;

      destPacket->hostSystemContext2 = (fiftyCC) << PULSAR_50
      | (hundredCC) << PULSAR_100
      | (fourCC) << PULSAR_400
      | (ninetyCC) << PULSAR_99999
      | (itemModeRain) << PULSAR_MODE_ITEMRAIN
      | (riiBalanced) << PULSAR_MODE_RIIBALANCED
      | (bumperKart) << PULSAR_MODE_BUMPERKARTS
      | (itemModeUnknown) << PULSAR_MODE_UNKNOWN
      | (itemModeBattleRoyale) << PULSAR_BATTLEROYALE
      | (worldwideMKDS) << PULSAR_WWMKDS
      | (worldwideItemRain) << PULSAR_WWITEMRAIN
      | (worldwideMayhem) << PULSAR_WWMAYHEM
      | (boxSpawnFast) << PULSAR_FASTBOX
      | (boxSpawnInstant) << PULSAR_INSTANTBOX
      | (boxSpawnDisabled) << PULSAR_DISABLEBOX
      | (settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_THUNDERCLOUD) << PULSAR_THUNDERCLOUD)
      | (settings.GetSettingValue(Settings::SETTINGSTYPE_ITEM, ITEM_FLYINGBLOOP) << PULSAR_FLYINGBLOOP)
      | (settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_HOSTWINS) << PULSAR_HAW);
  
        u8 raceCount;
        if (koSetting == KOSETTING_ENABLED) raceCount = 0xFE;
        else switch (settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_SCROLL_GP_RACES)) {
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
        (void)ApplyHostContextLocally(destPacket->hostSystemContext);
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

        // Get context from host packet (no need to read local settings - host values take precedence)
        Network::Mgr& netMgr = Pulsar::System::sInstance->netMgr;

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
        const u8 hostParam = Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_RULES, SETTINGHOST_RADIO_HOSTWINS);
        packetReg.packet.message |= hostParam << 2; //uses bit 2 of message

        const u8 gpParam = Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_RULES, SETTINGHOST_SCROLL_GP_RACES);
        const u8 disableMiiHeads = Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_RULES, SETTINGHOST_ALLOW_MIIHEADS);
        packetReg.packet.message |= gpParam << 3; //uses bits 3-5
        packetReg.packet.message |= disableMiiHeads << 6; //uses bit 6
        packetReg.packet.message |= Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_OTT, OTT_ONLINE) << 7; //7 for OTT
        packetReg.packet.message |= Settings::Mgr::GetSettingValue(Settings::SETTINGSTYPE_KO, KO_ENABLED) << 8; //8 for KO

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