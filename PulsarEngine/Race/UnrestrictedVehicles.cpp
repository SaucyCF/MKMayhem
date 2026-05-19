#include <kamek.hpp>
#include <RuntimeWrite.hpp>

namespace Pulsar {
namespace Race {

kmWrite32(0x8081CF58, 0x7E238B78);
kmWrite32(0x80832B8C, 0x38E00024);
kmWrite32(0x80832104, 0x38600000);
kmWrite32(0x8083E1B0, 0x38600000);
kmWrite32(0x808450D4, 0x931F0A08);
kmWrite32(0x80845598, 0x38000000);
kmWrite32(0x808455A4, 0x38000000);
kmWrite32(0x808455bc, 0x38600001);
kmWrite32(0x80845950, 0x60000000);
kmWrite32(0x808459A4, 0x60000000);
kmWrite32(0x808459B4, 0x2C150001);
kmWrite32(0x808459C0, 0x60000000);
kmWrite16(0x80845C44, 0x00004800);
kmWrite32(0x808478F4, 0x4E800020);
kmWrite32(0x8084A2E8, 0x38800000);
kmWrite32(0x8084A130, 0x38000000);
kmWrite32(0x8084A430, 0x38600000);
kmWrite32(0x8084FED4, 0x40820060);

asmFunc UVC1() {
  ASM(
  nofralloc;
  mfctr     r4;
  cmplwi    r4, 0;
  li        r4, 0x7;
  bne-      loc_0x20;
  andi.     r4, r18, 0x6;
  li        r4, 0x11;
  bne-      loc_0x20;
  li        r4, 0x12;

loc_0x20:
  blr;
  )
}
kmCall(0x805BE108, UVC1);

extern "C" void UVCEnd2(void*);
extern "C" void UVCSymbol1(void*);
extern "C" void UVCSymbol2(void*);
asmFunc UVC2() {
  ASM(
  nofralloc;
  lis       r12, UVCSymbol1@h;
  ori       r12, r12, UVCSymbol1@l;
  mfctr     r3;
  cmplw     r3, r12;
  bne-      loc_0x4C;
  cmpwi     r24, 0;
  bne-      loc_0x2C;
  lis       r12, UVCSymbol2@h;
  ori       r12, r12, UVCSymbol2@l;
  mtctr     r12;
  bctr;

loc_0x2C:
  andi.     r3, r18, 0x9;
  beq-      loc_0x40;
  cmplw     r20, r24;
  blt-      loc_0x4C;
  b         loc_0x48;

loc_0x40:
  cmplw     r20, r24;
  bgt-      loc_0x4C;

loc_0x48:
  mr        r20, r24;

loc_0x4C:
  stw       r20, 0x0(r23);
  b UVCEnd2;
  )
}
kmBranch(0x805F15D0, UVC2);

asmFunc UVC3() {
  ASM(
  nofralloc;
  lbz       r3, 0x15(r6);
  cmpwi     r3, 0x33;
  bne-      loc_0x14;
  li        r3, 0x55;
  stb       r3, 0x15(r6);

loc_0x14:
  li        r3, 0x174;
  blr;
  )
}
kmCall(0x80845238, UVC3);

asmFunc UVC4() {
  ASM(
  nofralloc;
  lbz       r4, 0x25(r3);
  cmpwi     r4, 0x33;
  bne-      loc_0x14;
  li        r4, 0x55;
  stb       r4, 0x25(r3);

loc_0x14:
  li        r4, 0x14;
  blr;
  )
}
kmCall(0x80845308, UVC4);

extern "C" void UVCSymbol3(void*);
asmFunc UVC5() {
  ASM(
  nofralloc;
  cmplwi    r23, 0x1;
  bne-      loc_0x24;
  cmplwi    r25, 0x1;
  bne-      loc_0x18;
  lis       r9, UVCSymbol3@ha;
  stw       r28, UVCSymbol3@l(r9);

loc_0x18:
  li        r9, 0x30;
  add       r9, r9, r25;
  stb       r9, 0xF(r5);

loc_0x24:
  li        r9, 0;
  blr;
  )
}
kmCall(0x808453C4, UVC5);

extern "C" void sInstance__8Racedata(void*);
extern "C" void UVCSymbol4(void*);
extern "C" void UVCSymbol5(void*);
asmFunc UVC6() {
  ASM(
  nofralloc;
  lis       r12, sInstance__8Racedata@ha;
  lwz       r12, sInstance__8Racedata@l(r12);
  lwz       r31, 0x1760(r12);
  lis       r12, UVCSymbol4@ha;
  b         loc_0x24;
  ori       r12, r12, 0x3;

loc_0x24:
  lbz       r31, UVCSymbol4@l(r12);
  stb       r31, UVCSymbol5@l(r12);
  stw       r31, 0x13C(r3);
  blr;
  )
}
kmCall(0x80846CF8, UVC6);

extern "C" void UVCEnd7(void*);
extern "C" void UVCSymbol6(void*);
extern "C" void UVCSymbol7(void*);
extern "C" void UVCSymbol8(void*);
extern "C" void UVCSymbol9(void*);
extern "C" void UVCSymbolA(void*);
extern "C" void SetMessage__15LayoutUIControlFUiPCQ24Text4Info(void*);
asmFunc UVC7() {
  ASM(
  nofralloc;
  stwu      r1, -0x80(r1);
  mflr      r12;
  stmw      r3, 0x8(r1);
  lis       r12, sInstance__8Racedata@ha;
  lwz       r12, sInstance__8Racedata@l(r12);
  lwz       r6, 0x1760(r12);
  lis       r12, UVCSymbol5@ha;
  li        r5, 0x23;
  b         loc_0x48;

loc_0x30:
  ori       r12, r12, 0x3;
  li        r5, 0x5;
  bl        loc_0x44;
  opword    0x00010212;
  opword		0x13140000;

loc_0x44:
  mflr      r6;

loc_0x48:
  lbz       r7, UVCSymbol5@l(r12);
  cmplwi    r18, 0;
  lbz       r4, UVCSymbol6@l(r12);
  beq-      loc_0x84;
  andi.     r30, r18, 0x6;
  bne-      loc_0x68;
  subi      r4, r4, 0x1;
  b         loc_0x6C;

loc_0x68:
  addi      r4, r4, 0x1;

loc_0x6C:
  cmpwi     r4, 0;
  bge-      loc_0x78;
  mr        r4, r5;

loc_0x78:
  cmpw      r4, r5;
  ble-      loc_0x84;
  li        r4, 0;

loc_0x84:
  cmpwi     r5, 0x5;
  mr        r7, r4;
  bne-      loc_0x94;
  lbzx      r7, r6, r4;

loc_0x94:
  stb       r4, UVCSymbol6@l(r12);
  stb       r7, UVCSymbol4@l(r12);
  stw       r7, 0x6F4(r3);
  addi      r4, r4, 0x1;
  addi      r5, r5, 0x1;
  lis       r12, UVCSymbol7@ha;
  stw       r4, UVCSymbol7@l(r12);
  stw       r5, UVCSymbol8@l(r12);
  lwz       r3, UVCSymbol9@l(r12);
  li        r4, 0x7D9;
  subis     r5, r12, 0x1;
  ori       r5, r5, UVCSymbolA@l;
  lis       r12, SetMessage__15LayoutUIControlFUiPCQ24Text4Info@h;
  ori       r12, r12, SetMessage__15LayoutUIControlFUiPCQ24Text4Info@l;
  mtctr     r12;
  bctrl;     
  lmw       r3, 0x8(r1);
  mtlr      r12;
  addi      r1, r1, 0x80;
  b UVCEnd7;
  )
}
kmBranch(0x80846ED4, UVC7);

extern "C" void UVCEnd8(void*);
asmFunc UVC8() {
  ASM(
  nofralloc;
  stwu      r1, -0x80(r1);
  stmw      r3, 0x8(r1);
  lis       r12, sInstance__8Racedata@ha;
  lwz       r12, sInstance__8Racedata@l(r12);
  lwz       r6, 0x1760(r12);
  li        r5, 0x23;
  b         loc_0x3C;

loc_0x28:
  li        r5, 0x5;
  bl        loc_0x38;
  opword    0x00010212;
  opword		0x13140000;

loc_0x38:
  mflr      r6;

loc_0x3C:
  lbz       r7, 0x6DB(r3);
  cmplwi    r17, 0;
  lbz       r30, 0x6E8(r3);
  beq-      loc_0x78;
  andi.     r12, r17, 0x1;
  beq-      loc_0x5C;
  subi      r30, r30, 0x1;
  b         loc_0x60;

loc_0x5C:
  addi      r30, r30, 0x1;

loc_0x60:
  cmpwi     r30, 0;
  bge-      loc_0x6C;
  mr        r30, r5;

loc_0x6C:
  cmpw      r30, r5;
  ble-      loc_0x78;
  li        r30, 0;

loc_0x78:
  cmpwi     r5, 0x5;
  mr        r7, r30;
  bne-      loc_0x88;
  lbzx      r7, r6, r30;

loc_0x88:
  stb       r7, 0x6DB(r3);
  stb       r30, 0x6E8(r3);
  mr        r0, r7;
  lmw       r3, 0x8(r1);
  addi      r1, r1, 0x80;
  mr        r30, r0;
  b UVCEnd8;
  )
}
kmBranch(0x8084A2FC, UVC8);

asmFunc UVC9() {
  ASM(
  nofralloc;
  cmpwi     r9, -0x1;
  beq-      loc_0x14;
  lis       r12, 0x9000;
  cmplw     r9, r12;
  bge-      loc_0x18;

loc_0x14:
  mr        r3, r27;

loc_0x18:
  cmpw      r3, r27;
  blr;
  )
}
kmCall(0x8084A32C, UVC9);

asmFunc UVCA() {
  ASM(
  nofralloc;
  lwz       r20, 0xF4(r4);
  lbz       r19, 0x33(r4);
  cmpwi     r19, 0x3;
  blt-      loc_0x24;
  cmpwi     r19, 0x12;
  blt-      loc_0x20;
  cmpwi     r19, 0x15;
  blt-      loc_0x24;

loc_0x20:
  li        r20, 0x2;

loc_0x24:
  blr;
  )
}
kmCall(0x80553FC4, UVCA);

//Free up allocated memory for allkart since that much is no longer needed
kmWrite32(0x805422F0, 0x3CA00001);
kmWrite32(0x80542310, 0x3F400001);

}//namespace Race
}//namespace Pulsar