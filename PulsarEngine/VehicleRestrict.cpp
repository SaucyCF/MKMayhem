#include <DKW.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/UI/Ctrl/UIControl.hpp>
#include <Network/MatchCommand.hpp>
#include <MarioKartWii/UI/Page/Menu/KartSelect.hpp>

//DKW Dev Note: Code by Retro Rewind and WTP Teams

namespace DKW{
namespace UI{

    u8 RestrictVehicleSelection(){
        SectionMgr::sInstance->sectionParams->kartsDisplayType = 2;
        bool kartRestrict = Pulsar::DKWSETTING_VEHICLERESTRICT_DEFAULT;
        bool bikeRestrict = Pulsar::DKWSETTING_VEHICLERESTRICT_DEFAULT;

        if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST){
            kartRestrict = System::sInstance->IsContext(Pulsar::PULSAR_KARTRESTRICT) ? Pulsar::DKWSETTING_VEHICLERESTRICT_KARTS : Pulsar::DKWSETTING_VEHICLERESTRICT_DEFAULT;
            bikeRestrict = System::sInstance->IsContext(Pulsar::PULSAR_BIKERESTRICT) ? Pulsar::DKWSETTING_VEHICLERESTRICT_BIKES : Pulsar::DKWSETTING_VEHICLERESTRICT_DEFAULT;
        }

        const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
        const GameMode mode = scenario.settings.gamemode;

        if(kartRestrict == Pulsar::DKWSETTING_VEHICLERESTRICT_KARTS){
            SectionMgr::sInstance->sectionParams->kartsDisplayType = 0;
        }
        if(bikeRestrict == Pulsar::DKWSETTING_VEHICLERESTRICT_BIKES){
            SectionMgr::sInstance->sectionParams->kartsDisplayType = 1;
        }

        return SectionMgr::sInstance->sectionParams->kartsDisplayType;
    }
    kmCall(0x808455a4, RestrictVehicleSelection);
    kmWrite32(0x808455a8, 0x907f06ec);

    bool isKartAccessible(KartId kart, u32 r4){
        bool ret = IsKartUnlocked(kart, r4);
        bool kartRestrict = Pulsar::DKWSETTING_VEHICLERESTRICT_DEFAULT;
        bool bikeRestrict = Pulsar::DKWSETTING_VEHICLERESTRICT_DEFAULT;

        if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST){
            kartRestrict = System::sInstance->IsContext(Pulsar::PULSAR_KARTRESTRICT) ? Pulsar::DKWSETTING_VEHICLERESTRICT_KARTS : Pulsar::DKWSETTING_VEHICLERESTRICT_DEFAULT;
            bikeRestrict = System::sInstance->IsContext(Pulsar::PULSAR_BIKERESTRICT) ? Pulsar::DKWSETTING_VEHICLERESTRICT_BIKES : Pulsar::DKWSETTING_VEHICLERESTRICT_DEFAULT;
        }

        if((kart < STANDARD_BIKE_S && bikeRestrict == Pulsar::DKWSETTING_VEHICLERESTRICT_BIKES) || (kart >= STANDARD_BIKE_S && kartRestrict == Pulsar::DKWSETTING_VEHICLERESTRICT_KARTS)){
            ret = false;
        }

        return ret;
    }
    kmCall(0x8084a45c, isKartAccessible);
}
}