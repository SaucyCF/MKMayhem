#include <DKW.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <runtimeWrite.hpp>

namespace Pulsar {
namespace ItemRain {

// Allow POWs and Shocks to affect other players on their screens.
asmFunc ItemRainOnlineFix() {
    ASM(
        loc_0x0 : addi r3, r4, 200;
        lis r12, 0x8000;
        lbz r12, 0x120C(r12);
        cmpwi r12, 0;
        beq loc_0x2C;
        mr r3, r4;
        b loc_0x2C;

        loc_0x2C:  )
}
kmCall(0x8065BB40, ItemRainOnlineFix);

// Fix bombs not activating offline [Ro]
asmFunc ItemRainOfflineFix() {
    ASM(
        loc_0x0:
            lwz r4, 0x4(r30);
            cmpwi r4, 0x9;
            bne- loc_0x18;
            li r0, 0x12C;
            stw r0, 0x1DC(r30);
            lwz r0, -0xFC(r5);

        loc_0x18:
            stw r0, 0x170(r30);
            blr;
    )
}
kmCall(0x807A7170, ItemRainOfflineFix);

// Anti Online Item Delimiters [Ro]
asmFunc GetItemDelimiterShock() {
    ASM(
        nofralloc;
        loc_0x0 : mflr r12;
        cmpwi r7, 0x1;
        bne + validLightning;
        addi r12, r12, 0x12C;
        mtlr r12;
        blr;
        validLightning : mulli r29, r3, 0xF0;
        blr;)
}

asmFunc GetItemDelimiterBlooper() {
    ASM(
        nofralloc;
        loc_0x0 : mflr r12;
        cmpwi r7, 0x1;
        bne + validBlooper;
        addi r12, r12, 0x1A8;
        mtlr r12;
        blr;
        validBlooper : addi r11, r1, 0x50;
        blr;)
}

asmFunc GetItemDelimiterPOW() {
    ASM(
        nofralloc;
        loc_0x0 : mflr r12;
        cmpwi r7, 0x1;
        bne + validPOW;
        addi r12, r12, 0x48;
        mtlr r12;
        blr;
        validPOW : mr r30, r3;
        blr;)
}

kmRuntimeUse(0x808A5D47);
kmRuntimeUse(0x808A5A3F);
kmRuntimeUse(0x808A538F);
kmRuntimeUse(0x808A56EB);
kmRuntimeUse(0x808A548B);
kmRuntimeUse(0x807B7C34);
kmRuntimeUse(0x807A81C0);
kmRuntimeUse(0x807B1B44);
kmRuntimeUse(0x807BB380);
kmRuntimeUse(0x807BB384);
kmRuntimeUse(0x8065b870);
void ItemRainFix() {
    ItemRainOnlineFixHook = 0x00;
    kmRuntimeWrite8A(0x808A5D47, 0x0000000c);
    kmRuntimeWrite8A(0x808A5A3F, 0x00000008);
    kmRuntimeWrite8A(0x808A538F, 0x00000010);
    kmRuntimeWrite8A(0x808A56EB, 0x00000006);
    kmRuntimeWrite8A(0x808A548B, 0x00000003);
    kmRuntimeCallA(0x807B7C34, GetItemDelimiterShock);
    kmRuntimeCallA(0x807A81C0, GetItemDelimiterBlooper);
    kmRuntimeCallA(0x807B1B44, GetItemDelimiterPOW);
    kmRuntimeWrite32A(0x807BB380, 0x7C0500D0);
    kmRuntimeWrite32A(0x807BB384, 0x2C840006);
    kmRuntimeWrite32A(0x8065b870, 0x38C600C8);
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_JOINING_REGIONAL) {
        if (Pulsar::System::sInstance->IsContext(PULSAR_MODE_ITEMRAIN) || Pulsar::System::sInstance->IsContext(PULSAR_MODE_MAYHEM)) {
            ItemRainOnlineFixHook = 0x00FF0100;
            kmRuntimeWrite8A(0x808A5D47, 0x00000022);
            kmRuntimeWrite8A(0x808A5A3F, 0x00000022);
            kmRuntimeWrite8A(0x808A538F, 0x00000022);
            kmRuntimeWrite8A(0x808A56EB, 0x00000019);
            kmRuntimeWrite8A(0x808A548B, 0x00000019);
            kmRuntimeWrite32A(0x807B7C34, 0x1FA300F0);
            kmRuntimeWrite32A(0x807A81C0, 0x39610050);
            kmRuntimeWrite32A(0x807B1B44, 0x7C7E1B78);
            kmRuntimeWrite32A(0x807BB380, 0x38600000);
            kmRuntimeWrite32A(0x807BB384, 0x4E800020);
            kmRuntimeWrite32A(0x8065b870, 0x38C60002);

        }
    }
}
static SectionLoadHook FixItemRain(ItemRainFix);

} // namespace ItemRain
} // namespace Pulsar