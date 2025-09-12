#include <kamek.hpp>
#include <MarioKartWii/Item/ItemBehaviour.hpp>
#include <MarioKartWii/Item/Obj/ObjProperties.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Kart/KartDamage.hpp>
#include <DKW.hpp>

//DKW Dev Note: Code by Retro Rewind and WTP Teams

namespace DKW{
namespace Race{

    static void ChangeBlueProp(Item::ObjProperties* dest, const Item::ObjProperties& rel)
    {
        bool itemModeRandom = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        bool itemModeShellShock = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST){
            itemModeRandom = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODERANDOM) ? Pulsar::DKWSETTING_GAMEMODE_RANDOM : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
            itemModeShellShock = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODESHELLSHOCK) ? Pulsar::DKWSETTING_GAMEMODE_SHELLSHOCK : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        }
        new (dest) Item::ObjProperties(rel);
        if(itemModeRandom == Pulsar::DKWSETTING_GAMEMODE_RANDOM){
            dest->limit = 5;
        }
        new (dest) Item::ObjProperties(rel);
        if(itemModeShellShock == Pulsar::DKWSETTING_GAMEMODE_SHELLSHOCK){
            dest->limit = 24;
        }
    }
    kmCall(0x80790b74, ChangeBlueProp);

    static void ChangeBulletProp(Item::ObjProperties* dest, const Item::ObjProperties& rel)
    {
        bool itemModeRandom = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        bool itemModeShellShock = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST){
            itemModeRandom = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODERANDOM) ? Pulsar::DKWSETTING_GAMEMODE_RANDOM : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
            itemModeShellShock = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODESHELLSHOCK) ? Pulsar::DKWSETTING_GAMEMODE_SHELLSHOCK : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        }
        new (dest) Item::ObjProperties(rel);
        if(itemModeRandom == Pulsar::DKWSETTING_GAMEMODE_RANDOM){
            dest->limit = 25;
        }
        new (dest) Item::ObjProperties(rel);
        if(itemModeShellShock == Pulsar::DKWSETTING_GAMEMODE_SHELLSHOCK){
            dest->limit = 6;
        }
    }
    kmCall(0x80790bf4, ChangeBulletProp);

    static void ChangeBombProp(Item::ObjProperties* dest, const Item::ObjProperties& rel)
    {
        bool itemModeRandom = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST){
            itemModeRandom = System::sInstance->IsContext(Pulsar::PULSAR_GAMEMODERANDOM) ? Pulsar::DKWSETTING_GAMEMODE_RANDOM : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        }
        new (dest) Item::ObjProperties(rel);
        if(itemModeRandom == Pulsar::DKWSETTING_GAMEMODE_RANDOM){
            dest->limit = 20;
        }
    }
    kmCall(0x80790bb4, ChangeBombProp);

} // namespace Race   
} // namespace DKW
