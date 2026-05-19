#include <DKW.hpp>

namespace Codes {

//HUD Color [Spaghetti Noppers]
kmWrite32(0x80895CC0, 0x00FF0000);
kmWrite32(0x80895CC4, 0x000000FF);
kmWrite32(0x80895CC8, 0x00FF0000);
kmWrite32(0x80895CCC, 0x000000FF);
kmWrite32(0x80895CD0, 0x00FF0000);
kmWrite32(0x80895CD4, 0x00000046);
kmWrite32(0x80895CD8, 0x00FF0000);
kmWrite32(0x80895CDC, 0x000000FF);
kmWrite32(0x80895CE0, 0x00FF0000);
kmWrite32(0x80895CE4, 0x000000FF);
kmWrite32(0x80895CE8, 0x00FF0000);
kmWrite32(0x80895CEC, 0x00000046);

//Remove Race HUD Gradient [Saucy]
kmWrite32(0x807EC170, 0x38630000);
kmWrite32(0x807EC18C, 0x38030000);
kmWrite32(0x807EC1A8, 0x38630000);

//Mii Outfit C Anti-Crash
kmWrite8(0x8089089D, 0x00000062);
kmWrite8(0x808908A9, 0x00000062);
kmWrite8(0x808908E5, 0x00000062);
kmWrite8(0x808908F1, 0x00000062);
kmWrite8(0x8089092D, 0x00000062);
kmWrite8(0x80890939, 0x00000062);

//Remove Worldwide Button [Chadderz]
kmWrite16(0x8064B982, 0x00000005);
kmWrite32(0x8064BA10, 0x60000000);
kmWrite32(0x8064BA38, 0x60000000);
kmWrite32(0x8064BA50, 0x60000000);
kmWrite32(0x8064BA5C, 0x60000000);
kmWrite16(0x8064BC12, 0x00000001);
kmWrite16(0x8064BC3E, 0x00000484);
kmWrite16(0x8064BC4E, 0x000010D7);
kmWrite16(0x8064BCB6, 0x00000484);
kmWrite16(0x8064BCC2, 0x000010D7);

// Skip Bubble Formation [Ro]
kmWrite32(0x805E2D14, 0x48000060);

// Prevent Lag Abuse [???]
kmWrite32(0x80654b00, 0x4E800020);

// Force player to not be penalized [B_squo]
kmWrite32(0x80549898, 0x38600000);
kmWrite32(0x8054989c, 0x4E800020);

// Disable Camera Shaking from Bombs [ZPL]
kmWrite32(0x805a906c, 0x4E800020);

// jugemnu_lap.brres [ZPL]
kmWrite16(0x808a22ec, 'KO');

// Disable 6 minute time limit Online [CLF78]
kmWrite32(0x8053F478, 0x4800000C);

//Don't Lose VR While Disconnecting [Bully]
kmWrite32(0x80856560, 0x60000000);

//Instant Voting Roulette Decide [Ro]
kmWrite32(0x80643BC4, 0x60000000);
kmWrite32(0x80643C2C, 0x60000000);

//No Sun Glare [Anarion]
kmWrite32(0x8054D260, 0x48000060);
kmWrite32(0x8054B690, 0x4082000C);

//Always Show Timer on Vote Screen by Chadderz
kmWrite32(0x80650254, 0x60000000);

//Allow pausing before the race starts by Sponge
kmWrite32(0x80856a28, 0x48000050);

//Show Nametags During Countdown By Ro
kmWrite32(0x807F13F0, 0x38600001);

//Allow All Vehicles in Battle Mode [Nameless, Scruffy]
kmWrite32(0x80553F98, 0x3880000A);
kmWrite32(0x8084FEF0, 0x48000044);
kmWrite32(0x80860A90, 0x38600000);

//No Disconnect on Countdown [_tZ]
kmWrite32(0x80655578, 0x60000000);

//Disable Opening Camera [2325]
kmWrite32(0x805A74A0, 0x480000AC);

//Change VR Limit [XeR]
kmWrite16(0x8052D286, 0x00007530);
kmWrite16(0x8052D28E, 0x00007530);
kmWrite16(0x8064F6DA, 0x00007530);
kmWrite16(0x8064F6E6, 0x00007530);
kmWrite16(0x8085654E, 0x00007530);
kmWrite16(0x80856556, 0x00007530);
kmWrite16(0x8085C23E, 0x00007530);
kmWrite16(0x8085C246, 0x00007530);
kmWrite16(0x8064F76A, 0x00007530);
kmWrite16(0x8064F776, 0x00007530);
kmWrite16(0x808565BA, 0x00007530);
kmWrite16(0x808565C2, 0x00007530);
kmWrite16(0x8085C322, 0x00007530);
kmWrite16(0x8085C32A, 0x00007530);

//Allow WFC on Wiimmfi Patched ISOs
kmWrite32(0x800EE3A0, 0x2C030000);
kmWrite32(0x800ECAAC, 0x7C7E1B78);

// Slot Specific Objects Work in All Slots (pylon01, sunDS, FireSnake and begoman_spike) [Ro]
kmWrite32(0x8082A4F8, 0x3800000A);

// Disable Data Save Reset for Region ID Change [Vega]
kmWrite32(0x80544928, 0x7C601B78);

// Mushroom Glitch Fix [Vabold]
kmWrite8(0x807BA077, 0x00);

// Allow Looking Backwards During Respawn
kmWrite32(0x805A228C, 0x60000000);

// Allow Mega in Mega
kmWrite32(0x807BB764, 0x60000000);

// Mega Flips Cars
kmWrite32(0x8082AC00, 0x3B800001);

asmFunc ItemLimitModifier() {
    ASM(
        nofralloc;
        li        r19, 0x10;
        stw       r19, 0x4(r4);
        blr;)
}
kmCall(0x80790E94, ItemLimitModifier);

//Anti Lag Start [Ro]
extern "C" void sInstance__8Racedata(void*);
asmFunc AntiLagStart(){
    ASM(
      nofralloc;
loc_0x0:
  lwz r12, sInstance__8Racedata@l(r30);
  lwz r12, 0xB70(r12);
  cmpwi r12, 0x7;
  blt- loc_0x14;
  li r3, 0x1;

loc_0x14:
  cmpwi r3, 0x0;
  blr;
  )
}
kmCall(0x80533430, AntiLagStart);

// Fix Online Players Stuck on Halfpipe (Halfpipe Warp Fix) [Ro]
asmFunc halfpipeWarpFix() {
    ASM(
        nofralloc;
        lwz r11, 8(r4);
        rlwinm.r12, r11, 0, 21, 21;
        beq - loc_0x20;
        lha r0, 86(r31);
        cmpwi r0, 0x52;
        blt - loc_0x20;
        rlwinm r11, r11, 0, 22, 20;
        stw r11, 8(r4);

        loc_0x20 :;
        mr r4, r11;
        blr;)
}
kmCall(0x8058BF58, halfpipeWarpFix);

// Anti Mii Crash
asmFunc AntiWiper() {
    ASM(
        nofralloc;
        loc_0x0 : cmpwi r4, 0x6;
        ble validMii;
        lhz r12, 0xE(r30);
        cmpwi r12, 0x0;
        bne validMii;
        li r31, 0x0;
        li r4, 0x6;
        validMii : mr r29, r4;
        blr;)
}
kmCall(0x800CB6C0, AntiWiper);
kmWrite32(0x80526660, 0x38000001);  // Credits to Ro for the last line.

// Anti Item Collission Crash [Marioiscool246]
extern "C" void __ptmf_test(void*);
asmFunc AntiItemColCrash() {
    ASM(
        nofralloc;
        loc_0x0 : stwu r1, -0xC(r1);
        stw r31, 8(r1);
        mflr r31;
        addi r3, r29, 0x174;
        bl __ptmf_test;
        cmpwi r3, 0;
        bne end;
        addi r31, r31, 0x14;

        end : mtlr r31;
        lwz r31, 8(r1);
        addi r1, r1, 0xC;
        mr r3, r29;
        blr;)
}
kmCall(0x807A1A54, AntiItemColCrash);

// Item Spam Anti-Freeze [???]
asmFunc ItemSpamAntiFreeze() {
    ASM(
        loc_0x0 : lbz r12, 0x1C(r27);
        add r12, r30, r12;
        cmpwi r12, 0xE0;
        blt + loc_0x18;
        li r0, 0;
        stb r0, 0x19(r27);

        loc_0x18 : lbz r0, 0x19(r27);)
}
kmCall(0x8065BBD4, ItemSpamAntiFreeze);

// Fix star offroad glitch after cannon [Ro]
asmFunc StarOffroadFix() {
    ASM(
        nofralloc;
        andi.r11, r0, 0x80;
        andis.r12, r0, 0x8000;
        or.r0, r11, r12;
        blr;)
}
kmCall(0x8057C3F8, StarOffroadFix);

// Respawn If Stuck on Top of Wall Collision [Ro]
asmFunc RespawnIfStuckFix() {
    ASM(
        nofralloc;
        lfs       f1, 0x4(r3);
        lwz       r3, 0x4(r31);
        lwz       r5, 0x8(r3);
        lwz       r6, 0x4(r3);
        lwz       r6, 0x4(r6);
        andi.     r7, r6, 0x8000;
        beq-      loc_0x5C;
        andi.     r6, r6, 0x60;
        beq-      loc_0x5C;
        lwz       r5, 0x90(r5);
        lwz       r5, 0x4(r5);
        lfs       f2, 0xE0(r5);
        lwz       r6, 0x0(r3);
        lbz       r5, 0x11(r6);
        lfs       f3, 0x428(r4);
        fcmpo     cr0, f2, f3;
        bge-      loc_0x54;
        addi      r5, r5, 0x1;
        cmpwi     r5, 0xF0;
        blt-      loc_0x58;
        lfs       f1, 0x6824(r4);
      loc_0x54:
        li        r5, 0;
      loc_0x58:
        stb       r5, 0x11(r6);
      loc_0x5C:
        blr;)
}
kmCall(0x80573F08, RespawnIfStuckFix);

asmFunc TrickableCannon() {
    ASM(
        nofralloc;
        ori       r0, r0, 0x10;
        oris      r0, r0, 0x4000;
        blr;)
}
kmCall(0x80584BB0, TrickableCannon);

} //namespace Codes