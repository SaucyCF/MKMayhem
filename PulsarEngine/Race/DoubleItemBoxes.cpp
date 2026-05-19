#include <kamek.hpp>
#include <RuntimeWrite.hpp>
#include <PulsarSystem.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <DKW.hpp>

namespace Pulsar {
namespace Race {

asmFunc DoubleItems1() {
  ASM(
  nofralloc;
  lwz       r0, 0x8C(r3);
  lbz       r12, 0xB3(r3);
  cmpwi     r12, 0;
  bne-      loc_0x60;
  lis       r12, 0x8000;
  cmplw     r27, r12;
  blt-      loc_0x38;
  lwz       r12, 0xA0(r27);
  cmpwi     r12, 0;
  beq-      loc_0x38;
  lwz       r12, 0x0(r12);
  lhz       r12, 0x38(r12);
  cmpwi     r12, 0x1;
  beq-      loc_0x58;

loc_0x38:
  lwz       r12, 0x5C(r3);
  cmpwi     r12, 0x5C;
  bge-      loc_0x58;
  lwz       r7, 0x58(r3);
  cmpwi     r7, 0;
  bne-      loc_0x60;
  cmpwi     r0, 0x14;
  beq-      loc_0x60;

loc_0x58:
  li        r7, 0x1;
  stb       r7, 0xB3(r3);

loc_0x60:
  blr;
  )
}

extern "C" void KartItemstartSpin(void*);
extern "C" void DoubleItemsEnd2(void*);
asmFunc DoubleItems2() {
  ASM(
  nofralloc;
  lwz       r0, 0x8C(r29);
  cmpwi     r0, 0x14;
  bne-      loc_0x64;
  lbz       r3, 0xB3(r29);
  cmpwi     r3, 0;
  beq-      loc_0x64;
  lwz       r3, 0x58(r29);
  cmpwi     r3, 0;
  bne-      loc_0x64;
  li        r3, 0;
  stb       r3, 0xB3(r29);
  stwu      r1, -0x80(r1);
  stmw      r3, 0x8(r1);
  mr        r3, r29;
  li        r4, 0;
  li        r5, 0;
  li        r6, 0;
  lis       r12, KartItemstartSpin@h;
  ori       r12, r12, KartItemstartSpin@l;
  mtctr     r12;
  bctrl;
  li        r12, 0x80;
  stw       r12, 0x5C(r29);
  lmw       r3, 0x8(r1);
  addi      r1, r1, 0x80;

loc_0x64:
  b DoubleItemsEnd2;
  )
}

asmFunc DoubleItems3() {
  ASM(
  nofralloc;
  stb       r0, 0x29(r28);
  stb       r0, 0x2B(r28);
  blr;
  )
}

asmFunc DoubleItems4() {
  ASM(
  nofralloc;
  stb       r0, 0x29(r31);
  stb       r0, 0x2B(r31);
  blr;
  )
}

extern "C" void DoubleItemsEnd5(void*);
asmFunc DoubleItems5() {
  ASM(
  nofralloc;
  bl        loc_0x24;

loc_0x4:
  opword    0x41200000;
  opword    0x42297AE1;
  opword    0xC2C2BD71;
  opword    0x4145EB85;

loc_0x14:
  opword    0x43E10000;
  opword    0xC1D90000;
  opword    0xC1910000;
  opword    0x3F800000;

loc_0x24:
  mflr      r11;
  lwz       r10, 0xA0(r28);
  lwz       r0, 0x758(r10);
  stw       r0, 0xB40(r10);
  stw       r0, 0x390(r10);
  stw       r0, 0x574(r10);
  lwz       r0, 0x75C(r10);
  stw       r0, 0xB44(r10);
  stw       r0, 0x578(r10);
  stw       r0, 0x394(r10);
  lwz       r0, 0x760(r10);
  stw       r0, 0xB48(r10);
  stw       r0, 0x57C(r10);
  stw       r0, 0x398(r10);
  lwz       r0, 0x764(r10);
  stw       r0, 0xB4C(r10);
  stw       r0, 0x580(r10);
  stw       r0, 0x39C(r10);
  li        r0, 0;
  lbz       r12, 0xB3(r3);
  cmpwi     r12, 0;
  bne-      loc_0xA4;
  andi.     r12, r4, 0x1;
  beq-      loc_0x90;
  lfs       f0, 0xA94(r10);
  fabs      f0, f0;
  stfs      f0, 0xA94(r10);

loc_0x90:
  lfs       f0, 0x4(r11);
  cmpwi     r4, 0x2;
  blt-      loc_0xD8;
  lfs       f0, 0x8(r11);
  b         loc_0xD8;

loc_0xA4:
  lfs       f0, 0xA98(r10);
  lfs       f1, 0x0(r11);
  lfs       f2, 0xC(r11);
  cmpwi     r4, 0x2;
  bge-      loc_0xC4;
  fcmpo     cr0, f0, f2;
  ble-      loc_0xD4;
  b         loc_0xD0;

loc_0xC4:
  fcmpo     cr0, f0, f2;
  bge-      loc_0xD4;
  fneg      f1, f1;

loc_0xD0:
  fsubs     f0, f0, f1;

loc_0xD4:
  li        r0, 0x1;

loc_0xD8:
  stfs      f0, 0xA98(r10);
  stb       r0, 0xB23(r10);
  li        r0, 0;
  lwz       r12, 0x8C(r3);
  cmpwi     r12, 0xA;
  beq-      loc_0xFC;
  lfs       f0, 0x18(r11);
  stfs      f0, 0x2E8(r10);
  b         loc_0x14C;

loc_0xFC:
  lfs       f0, 0x2E8(r10);
  lfs       f1, 0x1C(r11);
  lfs       f2, 0x14(r11);
  fcmpo     cr0, f0, f2;
  ble-      loc_0x118;
  fsubs     f0, f0, f1;
  stfs      f0, 0x2E8(r10);

loc_0x118:
  lha       r0, 0xAA(r3);
  cmpwi     r0, 0;
  bne-      loc_0x12C;
  lfs       f0, 0x10(r11);
  b         loc_0x144;

loc_0x12C:
  lfd       f0, 0x118(r30);
  stfd      f0, 0x0(r1);
  stw       r0, 0x4(r1);
  lfd       f1, 0x0(r1);
  fsub      f0, f1, f0;
  frsp      f0, f0;

loc_0x144:
  li        r0, 0x1;
  stfs      f0, 0x4E8(r10);

loc_0x14C:
  stb       r0, 0x373(r10);
  lwz       r0, 0x58(r3);
  b DoubleItemsEnd5;
  )
}

extern "C" void NextLimited__6RandomFi(void*);
extern "C" void DoubleItemsEnd6(void*);
asmFunc DoubleItems6() {
  ASM(
  nofralloc;
  mr        r29, r3;
  li        r4, 0x1;
  lis       r12, 0x6974;
  ori       r12, r12, 0x656D;
  lwz       r11, 0x0(r5);
  cmpw      r11, r12;
  bne-      loc_0x9C;
  lwz       r12, 0x8C(r3);
  cmpwi     r12, 0;
  beq-      loc_0x9C;
  lwz       r12, 0x0(r12);
  lhz       r11, 0x38(r12);
  cmpwi     r11, 0x2;
  beq-      loc_0x9C;
  stwu      r1, -0x80(r1);
  stmw      r4, 0x8(r1);
  bl        loc_0x4C;
  cmplwi    cr6, r0, 0xB1E5;
  opword    0x420000b5;

loc_0x4C:
  mflr      r3;
  lwz       r4, 0x4(r12);
  stw       r4, 0x0(r3);
  lwz       r4, 0xC(r12);
  stw       r4, 0x4(r3);
  subi      r3, r3, 0x8;
  li        r4, 0x5;
  lis       r12, NextLimited__6RandomFi@h;
  ori       r12, r12, NextLimited__6RandomFi@l;
  mtctr     r12;
  bctrl;
  lmw       r4, 0x8(r1);
  addi      r1, r1, 0x80;
  cmpwi     r3, 0;
  bne-      loc_0x90;
  li        r11, 0x1;
  sth       r11, 0x38(r12);

loc_0x90:
  cmpwi     r11, 0x1;
  bne-      loc_0x9C;
  li        r4, 0;

loc_0x9C:
  mr        r3, r29;
  b DoubleItemsEnd6;
  )
}

asmFunc DoubleItems7() {
  ASM(
  nofralloc;
  stw       r3, 0x4(r31);
  stb       r3, 0x5F(r31);
  blr;
  )
}
kmBranch(0x807EEFBC, DoubleItems5);



kmRuntimeUse(0x80798C58);
kmRuntimeUse(0x80797F04);
kmRuntimeUse(0x807BCA70);
kmRuntimeUse(0x807BC74C);
kmRuntimeUse(0x8081FDAC);
kmRuntimeUse(0x807BA61C);
void DoubleItemsToggle() {
  kmRuntimeWrite32A(0x80798C58, 0x8003008C);
  kmRuntimeWrite32A(0x80797F04, 0x801D008C);
  kmRuntimeWrite32A(0x807BCA70, 0x981C0029);
  kmRuntimeWrite32A(0x807BC74C, 0x981F0029);
  kmRuntimeWrite32A(0x8081FDAC, 0x38800001);
  kmRuntimeWrite32A(0x807BA61C, 0x907F0004);
  if(System::sInstance->IsContext(Pulsar::PULSAR_DOUBLE_ITEMBOX) || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL) {
    kmRuntimeCallA(0x80798C58, DoubleItems1);
    kmRuntimeBranchA(0x80797F04, DoubleItems2);
    kmRuntimeCallA(0x807BCA70, DoubleItems3);
    kmRuntimeCallA(0x807BC74C, DoubleItems4);
    kmRuntimeBranchA(0x8081FDAC, DoubleItems6);
    kmRuntimeCallA(0x807BA61C, DoubleItems7);
  }
}
SectionLoadHook DoubleItemBoxesHook(DoubleItemsToggle);

}//namespace Race
}//namespace Pulsar