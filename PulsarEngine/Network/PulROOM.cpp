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
        const u8 superPowers = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, KO_SUPERPOWER) == KOSETTING_SUPERPOWER_ENABLED;
        const u8 koFinal = settings.GetSettingValue(Settings::SETTINGSTYPE_KO, KO_FINAL) == KOSETTING_FINAL_ALWAYS;
        const u8 startMKDS = (originalMessage == 4);

        const u8 fiftyCC = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_CC) == HOSTSETTING_CC_50;
        const u8 hundredCC = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_CC) == HOSTSETTING_CC_REAL100;
        const u8 fourCC = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_CC) == HOSTSETTING_CC_400;
        const u8 ninetyCC = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_CC) == HOSTSETTING_CC_99999;
        const u8 ultras = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_ULTRAS) == DKWSETTING_ULTRAS_ENABLED;
        const u8 cantBrakeDrift = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_BDRIFTING) == DKWSETTING_150_BRAKEDRIFT_OFF;
        const u8 cantFastFall = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_FALLFAST) == DKWSETTING_150_FALLFASTOFF;
        const u8 cantAlwaysDrift = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_ALWAYSDRIFT) == DKWSETTING_ALLOW_DANYWHEREOFF;

        const u8 mayhemCodes = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_MAYHEM_CODES) == DKWSETTING_MAYHEM_MKDS;
        const u8 mayhemWorld = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_MAYHEM_CODES) == DKWSETTING_MAYHEM_WORLD;
        const u8 mayhemMK8U = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_MAYHEM_CODES) == DKWSETTING_MAYHEM_MK8U;
        const u8 itemModeUnknown = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_UNKNOWNITEMS;
        const u8 itemModeRain = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_ITEMRAIN;
        const u8 itemModeMayhem = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_MAYHEM;
        const u8 bumperKart = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_BUMPERKARTS;
        const u8 modeCountdown = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_GAMEMODE) == DKWSETTING_GAMEMODE_COUNTDOWN;
        const u8 disableInvisWalls = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_INVIS_WALLS) == DKWSETTING_INVISWALLS_DISABLED;
        const u8 booFullInvis = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_BOO_VISIBILITY) == DKWSETTING_BOO_VISIBILITY_FULL_INVIS;
        const u8 transmissionVanilla = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_VANILLA;
        const u8 transmissionInsideAll = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_INSIDEALL;
        const u8 transmissionOutsideAll = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_SCROLL_FORCETRANSMISSION) == DKWSETTING_FORCE_TRANSMISSION_OUTSIDEALL;
        const u8 boxSpawnFast = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_FASTSPAWN;
        const u8 boxSpawnInstant = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_INSTANTSPAWN;
        const u8 boxSpawnDisabled = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_ITEMBOXSPAWN) == DKWSETTING_ITEMBOX_DISABLED;
        const u8 boxCountDouble = settings.GetSettingValue(Settings::SETTINGSTYPE_RULES2, RULES_ITEMBOXCOUNT) == DKWSETTING_ITEMBOXCOUNT_DOUBLE;

        const u8 itemBoo = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEMS, ITEMS_BOO) == DKWSETTING_ITEM_ENABLED;
        const u8 itemFeather = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEMS, ITEMS_FEATHER) == DKWSETTING_ITEM_ENABLED;
        const u8 itemTripleFib = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEMS, ITEMS_TRIPLE_FIB) == DKWSETTING_ITEM_ENABLED;
        const u8 itemShroomStar = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEMS, ITEMS_SHROOM_STAR) == DKWSETTING_ITEM_ENABLED;
        const u8 itemShellMushroom = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEMS, ITEMS_GREEN_SHELL_MUSHROOM) == DKWSETTING_ITEM_ENABLED;
        const u8 itemBobombMushroom = settings.GetSettingValue(Settings::SETTINGSTYPE_ITEMS, ITEMS_BOB_OMB_MUSHROOM) == DKWSETTING_ITEM_ENABLED;

        destPacket->hostSystemContext = (koSetting << PULSAR_MODE_KO)
      | (lapKoSetting) << PULSAR_MODE_LAPKO
      | (superPowers) << PULSAR_SUPERPOWERS
      | (settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_ULTRAS) ^ true) << PULSAR_ULTRAS
      | (settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_ALLOW_MIIHEADS) ^ true) << PULSAR_MIIHEADS
      | (mayhemCodes) << PULSAR_MAYHEM
      | (transmissionVanilla) << PULSAR_TRANSMISSIONVANILLA
      | (transmissionInsideAll) << PULSAR_TRANSMISSIONINSIDEALL
      | (transmissionOutsideAll) << PULSAR_TRANSMISSIONOUTSIDEALL
      | (boxCountDouble) << PULSAR_DOUBLE_ITEMBOX
      | (cantBrakeDrift) << PULSAR_BDRIFTING
      | (cantFastFall) << PULSAR_FALLFAST
      | (cantAlwaysDrift) << PULSAR_NODRIFTANYWHERE
      | (disableInvisWalls) << PULSAR_INVISWALLS
      | (booFullInvis) << PULSAR_BOO_FULL_INVIS
      | (startMKDS) << PULSAR_STARTMKDS
      | (settings.GetSettingValue(Settings::SETTINGSTYPE_RULES, RULES_RADIO_HOSTWINS) << PULSAR_HAW);

      destPacket->hostSystemContext2 = (mayhemWorld << PULSAR_MAYHEM_WORLD)
      | (mayhemMK8U << PULSAR_MAYHEM_MK8U)
      | (fiftyCC) << PULSAR_50
      | (hundredCC) << PULSAR_100
      | (fourCC) << PULSAR_400
      | (ninetyCC) << PULSAR_99999
      | (itemModeRain) << PULSAR_MODE_ITEMRAIN
      | (itemModeMayhem) << PULSAR_MODE_MAYHEM
      | (bumperKart) << PULSAR_MODE_BUMPERKARTS
      | (itemModeUnknown) << PULSAR_MODE_UNKNOWN
      | (boxSpawnFast) << PULSAR_FASTBOX
      | (boxSpawnInstant) << PULSAR_INSTANTBOX
      | (boxSpawnDisabled) << PULSAR_DISABLEBOX
      | (itemBoo) << PULSAR_BOO
      | (itemFeather) << PULSAR_FEATHER
      | (itemTripleFib) << PULSAR_TRIPLEFIB
      | (itemShroomStar) << PULSAR_SHROOMSTAR
      | (itemShellMushroom) << PULSAR_SHELLMUSHROOM
      | (itemBobombMushroom) << PULSAR_BOBOMBMUSHROOM
      | (modeCountdown) << PULSAR_MODE_COUNTDOWN;
  
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