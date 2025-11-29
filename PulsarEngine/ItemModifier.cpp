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
        bool itemModeUnknown = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST){
            itemModeUnknown = System::sInstance->IsContext(Pulsar::PULSAR_MODE_UNKNOWN) ? Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        }
        new (dest) Item::ObjProperties(rel);
        if(itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS){
            dest->limit = 5;
        }
    }
    kmCall(0x80790b74, ChangeBlueProp);

    static void ChangeBulletProp(Item::ObjProperties* dest, const Item::ObjProperties& rel)
    {
        bool itemModeUnknown = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST){
            itemModeUnknown = System::sInstance->IsContext(Pulsar::PULSAR_MODE_UNKNOWN) ? Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        }
        new (dest) Item::ObjProperties(rel);
        if(itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS){
            dest->limit = 25;
        }
    }
    kmCall(0x80790bf4, ChangeBulletProp);

    static void ChangeBombProp(Item::ObjProperties* dest, const Item::ObjProperties& rel)
    {
        bool itemModeUnknown = Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST){
            itemModeUnknown = System::sInstance->IsContext(Pulsar::PULSAR_MODE_UNKNOWN) ? Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS : Pulsar::DKWSETTING_GAMEMODE_REGULAR;
        }
        new (dest) Item::ObjProperties(rel);
        if(itemModeUnknown == Pulsar::DKWSETTING_GAMEMODE_UNKNOWNITEMS){
            dest->limit = 20;
        }
    }
    kmCall(0x80790bb4, ChangeBombProp);

} // namespace Race   
} // namespace DKW
