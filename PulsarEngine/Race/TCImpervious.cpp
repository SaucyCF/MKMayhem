#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Item/Obj/Kumo.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>

namespace Pulsar {

extern "C" bool TCImpervious_HandleNewTC(Item::ObjKumo* kumo) {
    if (kumo->duration <= 60) return false;
    u8 playerId = kumo->playerUsedItemId;
    Item::Manager* mgr = Item::Manager::sInstance;
    if (mgr == nullptr || mgr->players == nullptr || playerId >= mgr->playerCount) return false;
    Item::Player& player = mgr->players[playerId];
    if (player.isRemote) return false;
    if (player.roulette.unknown_0x24 != (u32)THUNDER_CLOUD) return false;
    player.roulette.unknown_0x24 = (u32)ITEM_NONE;
    kumo->KillObj(0);
    return true;
}

extern "C" void ObjKumo_Update_Epilog();
asmFunc ThundercloudImpervious() {
    ASM(
        nofralloc;
        stwu      r1, -0x20(r1);
        mflr      r0;
        stw       r0, 0x24(r1);
        stw       r31, 0x1C(r1);
        mr        r3, r28;
        bl        TCImpervious_HandleNewTC;
        cmpwi     r3, 0x0;
        beq       keepImpervious;
        lwz       r0, 0x24(r1);
        mtlr      r0;
        lwz       r31, 0x1C(r1);
        addi      r1, r1, 0x20;
        lis       r12, ObjKumo_Update_Epilog @ha;
        addi      r12, r12, ObjKumo_Update_Epilog @l;
        mtctr     r12;
        bctr;

    keepImpervious:
        lwz       r0, 0x24(r1);
        mtlr      r0;
        lwz       r31, 0x1C(r1);
        addi      r1, r1, 0x20;
        li        r0, 0x0;
        blr;
    );
}
kmCall(0x807AFAA8, ThundercloudImpervious);
kmWrite32(0x807AFAB4, 0x4800002C);
kmWrite32(0x80584b70, 0x60000000);

} //namespace Pulsar