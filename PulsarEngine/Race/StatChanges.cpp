#include <kamek.hpp>
#include <MarioKartWii/Race/Raceinfo/Raceinfo.hpp>
#include <MarioKartWii/3D/Model/ModelDirector.hpp>
#include <MarioKartWii/Kart/KartValues.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <MarioKartWii/Item/Obj/ObjProperties.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/File/StatsParam.hpp>
#include <Race/200ccParams.hpp>
#include <PulsarSystem.hpp>
#include <DKW.hpp>

namespace Pulsar {
// ALL CREDIT GOES TO ZPL https://github.com/Retro-Rewind-Team/Pulsar/commit/8f7205debd2569420a901142e3cc05815f7a9a36
    
Kart::Stats* ApplyStatChanges(KartId kartId, CharacterId characterId, KartType kartType) {
    union SpeedModConv {
        float speedMod;
        u32 kmpValue;
    };

    Kart::Stats* stats = Kart::ComputeStats(kartId, characterId);
    const GameMode gameMode = Racedata::sInstance->menusScenario.settings.gamemode;
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const RKNet::RoomType roomType = RKNet::Controller::sInstance->roomType;
    SpeedModConv speedModConv;
    speedModConv.kmpValue = (KMP::Manager::sInstance->stgiSection->holdersArray[0]->raw->speedMod << 16);
    if(speedModConv.speedMod == 0.0f) speedModConv.speedMod = 1.0f;
    float factor = 1.0f;
    bool NINETY = Pulsar::HOSTSETTING_CC_NORMAL;
    bool FOUR = Pulsar::HOSTSETTING_CC_NORMAL;
    bool HUNDRED = Pulsar::HOSTSETTING_CC_NORMAL;
    bool FIFTY = Pulsar::HOSTSETTING_CC_NORMAL;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
        NINETY = System::sInstance->IsContext(Pulsar::PULSAR_99999) ? Pulsar::HOSTSETTING_CC_99999 : Pulsar::HOSTSETTING_CC_NORMAL;
        FOUR = System::sInstance->IsContext(Pulsar::PULSAR_400) ? Pulsar::HOSTSETTING_CC_400 : Pulsar::HOSTSETTING_CC_NORMAL;
        HUNDRED = System::sInstance->IsContext(Pulsar::PULSAR_100) ? Pulsar::HOSTSETTING_CC_REAL100 : Pulsar::HOSTSETTING_CC_NORMAL;
        FIFTY = System::sInstance->IsContext(Pulsar::PULSAR_50) ? Pulsar::HOSTSETTING_CC_50 : Pulsar::HOSTSETTING_CC_NORMAL;
    }
    if (NINETY == Pulsar::HOSTSETTING_CC_99999 && FOUR != Pulsar::HOSTSETTING_CC_400 && HUNDRED != Pulsar::HOSTSETTING_CC_REAL100 && FIFTY != Pulsar::HOSTSETTING_CC_50){
        factor = 15.64f;
    }
    else if (FOUR == Pulsar::HOSTSETTING_CC_400 && NINETY != Pulsar::HOSTSETTING_CC_99999 && HUNDRED != Pulsar::HOSTSETTING_CC_REAL100 && FIFTY != Pulsar::HOSTSETTING_CC_50){
        factor = 2.67f;
    }
    else if (HUNDRED == Pulsar::HOSTSETTING_CC_REAL100 && NINETY != Pulsar::HOSTSETTING_CC_99999 && FOUR != Pulsar::HOSTSETTING_CC_400 && FIFTY != Pulsar::HOSTSETTING_CC_50){
        factor = 1.1251f;
    }
    else if (FIFTY == Pulsar::HOSTSETTING_CC_50 && NINETY != Pulsar::HOSTSETTING_CC_99999 && FOUR != Pulsar::HOSTSETTING_CC_400 && HUNDRED != Pulsar::HOSTSETTING_CC_REAL100){
        factor = 1.0001f;
    }
    else if (System::sInstance->IsContext(PULSAR_200)){
        factor = Race::speedFactor;
    }
    else if (DKW::System::Is50cc() && gameMode == MODE_VS_RACE){
        factor = 1.0001f;
    }
    else if (DKW::System::Is100cc() && gameMode == MODE_VS_RACE){
        factor = 1.1251f;
    }
    else if (DKW::System::Is400cc() && gameMode == MODE_VS_RACE){
        factor = 2.67f;
    }
    else if (DKW::System::Is99999cc() && gameMode == MODE_VS_RACE){
        factor = 15.64f;
    }
    else if (DKW::System::Is400cc() && gameMode == MODE_BATTLE || DKW::System::Is400cc() && gameMode == MODE_PUBLIC_BATTLE || DKW::System::Is400cc() && gameMode == MODE_PRIVATE_BATTLE){
        factor = 1.214;
    }
    factor *= speedModConv.speedMod;

    bool isItemModeMayhem = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE) {
        isItemModeMayhem = System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM) ? Pulsar::DKWSETTING_GAMEMODE_MAYHEM : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
    }
    if (isItemModeMayhem == Pulsar::DKWSETTING_GAMEMODE_MAYHEM) {
        Item::greenShellSpeed = 210.0f * factor;
        Item::redShellInitialSpeed = 150.0f * factor;
        Item::redShellSpeed = 260.0f * factor;
        Item::blueShellSpeed = 520.0f * factor;
        Item::blueShellMinimumDiveDistance = 640000.0f * factor;
        Item::blueShellHomingSpeed = 260.0f * factor;

        Kart::hardSpeedCap = 180.0f * factor;
        Kart::bulletSpeed = 218.0f * factor;
        Kart::starSpeed = 158.0f * factor;
        Kart::megaTCSpeed = 143.0f * factor;
    } else {
        Item::greenShellSpeed = 105.0f * factor;
        Item::redShellInitialSpeed = 75.0f * factor;
        Item::redShellSpeed = 130.0f * factor;
        Item::blueShellSpeed = 260.0f * factor;
        Item::blueShellMinimumDiveDistance = 640000.0f * factor;
        Item::blueShellHomingSpeed = 130.0f * factor;

        Kart::hardSpeedCap = 120.0f * factor;
        Kart::bulletSpeed = 145.0f * factor;
        Kart::starSpeed = 105.0f * factor;
        Kart::megaTCSpeed = 95.0f * factor;
}

    stats->baseSpeed *= factor;
    stats->standard_acceleration_as[0] *= factor;
    stats->standard_acceleration_as[1] *= factor;
    stats->standard_acceleration_as[2] *= factor;
    stats->standard_acceleration_as[3] *= factor;
    
    bool isLocalPlayer = false;
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    bool isGhostPlayer = false;
    for (int playerId = 0; playerId < scenario.playerCount; ++playerId) {
        if (scenario.players[playerId].kartId == kartId && 
            scenario.players[playerId].characterId == characterId) {
            if(scenario.players[playerId].playerType == PLAYER_REAL_LOCAL) {
                isLocalPlayer = true;
            }
            else if (scenario.players[playerId].playerType == PLAYER_GHOST) {
                isGhostPlayer = true;
            }
        }
    }

    if(isGhostPlayer) {
        return stats; // don't apply transmission changes if ghost and player have the same combo
    }

    bool VanillaTM = Pulsar::DKWSETTING_FORCE_TRANSMISSION_DEFAULT;
    bool insideAll = Pulsar::DKWSETTING_FORCE_TRANSMISSION_DEFAULT;
    bool outsideAll = Pulsar::DKWSETTING_FORCE_TRANSMISSION_DEFAULT;
    const GameMode mode = scenario.settings.gamemode;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
        VanillaTM = System::sInstance->IsContext(Pulsar::PULSAR_TRANSMISSIONVANILLA) ? Pulsar::DKWSETTING_FORCE_TRANSMISSION_VANILLA : Pulsar::DKWSETTING_FORCE_TRANSMISSION_DEFAULT;
        insideAll = System::sInstance->IsContext(Pulsar::PULSAR_TRANSMISSIONINSIDEALL) ? Pulsar::DKWSETTING_FORCE_TRANSMISSION_INSIDEALL : Pulsar::DKWSETTING_FORCE_TRANSMISSION_DEFAULT;
        outsideAll = System::sInstance->IsContext(Pulsar::PULSAR_TRANSMISSIONOUTSIDEALL) ? Pulsar::DKWSETTING_FORCE_TRANSMISSION_OUTSIDEALL : Pulsar::DKWSETTING_FORCE_TRANSMISSION_DEFAULT;
    }
    u32 transmission = static_cast<Pulsar::DKWSettingTransmission>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC2), Pulsar::MISC_TRANSMISSION));
    if (RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_VS_WW && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_BT_WW && isLocalPlayer) {
         if (mode == MODE_TIME_TRIAL || mode == MODE_GHOST_RACE) {
            if (stats->type == INSIDE_BIKE) {
                stats->type = INSIDE_BIKE;
            } else if (stats->type == KART) {
                stats->type = KART;
            } else if (stats->type == OUTSIDE_BIKE) {
                stats->type = OUTSIDE_BIKE;
            }
        }
        else if (insideAll == Pulsar::DKWSETTING_FORCE_TRANSMISSION_INSIDEALL && (roomType == RKNet::ROOMTYPE_FROOM_HOST || roomType == RKNet::ROOMTYPE_FROOM_NONHOST)) {
            if (stats->type == INSIDE_BIKE) {
                stats->type = INSIDE_BIKE;
                stats->targetAngle = 0.0f;
            } else if (stats->type == KART) {
                stats->type = INSIDE_BIKE;
                stats->mt += 20.0f;
            } else if (stats->type == OUTSIDE_BIKE) {
                stats->type = INSIDE_BIKE;
                stats->targetAngle = 0.0f;
            }
        }
        else if (outsideAll == Pulsar::DKWSETTING_FORCE_TRANSMISSION_OUTSIDEALL && (roomType == RKNet::ROOMTYPE_FROOM_HOST || roomType == RKNet::ROOMTYPE_FROOM_NONHOST)) {
            if (stats->type == INSIDE_BIKE) {
                stats->type = OUTSIDE_BIKE;
                stats->targetAngle = 45.0f;
            } else if (stats->type == KART) {
                stats->type = KART;
            } else if (stats->type == OUTSIDE_BIKE) {
                stats->type = OUTSIDE_BIKE;
                stats->targetAngle = 45.0f;
            }
        }
        else if (VanillaTM == Pulsar::DKWSETTING_FORCE_TRANSMISSION_VANILLA && (roomType == RKNet::ROOMTYPE_FROOM_HOST || roomType == RKNet::ROOMTYPE_FROOM_NONHOST)) {
            if (stats->type == INSIDE_BIKE) {
                stats->type = INSIDE_BIKE;
            } else if (stats->type == KART) {
                stats->type = KART;
            } else if (stats->type == OUTSIDE_BIKE) {
                stats->type = OUTSIDE_BIKE;
            }
        }
        else if (transmission == Pulsar::DKWSETTING_TRANSMISSION_ALLINSIDE)
        {
            if (stats->type == INSIDE_BIKE) {
                stats->type = INSIDE_BIKE;
                stats->targetAngle = 0.0f;
            } else if (stats->type == KART) {
                stats->type = INSIDE_BIKE;
                stats->mt += 20.0f;
            } else if (stats->type == OUTSIDE_BIKE) {
                stats->type = INSIDE_BIKE;
                stats->targetAngle = 0.0f;
            }
        }
        else if (transmission == Pulsar::DKWSETTING_TRANSMISSION_BIKEINSIDE)
        {
            if (stats->type == INSIDE_BIKE) {
                stats->type = INSIDE_BIKE;
                stats->targetAngle = 0.0f;
            } else if (stats->type == KART) {
                stats->type = KART;
            } else if (stats->type == OUTSIDE_BIKE) {
                stats->type = INSIDE_BIKE;
                stats->targetAngle = 0.0f;
            }
        }
        else if (transmission == Pulsar::DKWSETTING_TRANSMISSION_ALLOUTSIDE)
        {
            if (stats->type == INSIDE_BIKE) {
                stats->type = OUTSIDE_BIKE;
                stats->targetAngle = 45.0f;
            } else if (stats->type == KART) {
                stats->type = KART;
            } else if (stats->type == OUTSIDE_BIKE) {
                stats->type = OUTSIDE_BIKE;
                stats->targetAngle = 45.0f;
            }
        }
    }

    // Can Drift Anywhere 
    bool NoDriftAnywhere = Pulsar::DKWSETTING_ALLOW_DANYWHEREON;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST) {
        NoDriftAnywhere = System::sInstance->IsContext(Pulsar::PULSAR_NODRIFTANYWHERE) ? Pulsar::DKWSETTING_ALLOW_DANYWHEREOFF : Pulsar::DKWSETTING_ALLOW_DANYWHEREON;
    }

    if (NoDriftAnywhere == Pulsar::DKWSETTING_ALLOW_DANYWHEREOFF || mode == MODE_TIME_TRIAL || mode == MODE_GHOST_RACE || mode == MODE_PUBLIC_VS) {
    Kart::minDriftSpeedRatio = 0.55f * (factor > 1.0f ? (1.0f / factor) : 1.0f);}
    else if (static_cast<Pulsar::DKWSettingDriftAnywhere>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC2), Pulsar::MISC_ALWAYSDRIFT)) == Pulsar::DKWSETTING_DANYWHERE_ENABLED) {
    Kart::minDriftSpeedRatio = 0.001f * (factor > 1.0f ? (1.0f / factor) : 1.0f);}
    Kart::unknown_70 = 70.0f * factor;
    Kart::regularBoostAccel = 3.0f * factor;

    return stats;
}
kmCall(0x8058f670, ApplyStatChanges);

} //namespace Pulsar