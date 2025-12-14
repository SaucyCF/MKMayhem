#include <kamek.hpp>
#include <runtimeWrite.hpp>
#include <Settings/SettingsParam.hpp>
#include <MarioKartWii/Race/Racedata.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <Settings/UI/SettingsPanel.hpp>
#include <Settings/Settings.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/Objects/Collidable/Itembox/Itembox.hpp>
#include <PulsarSystem.hpp>
#include <DKW.hpp>
#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>

namespace DKW {

void ItemBoxRespawn(Objects::Itembox* itembox) {
    bool isFastBox = Pulsar::DKWSETTING_ITEMBOX_DEFAULT;
    bool isInstantBox = Pulsar::DKWSETTING_ITEMBOX_DEFAULT;
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE) {
        isFastBox = Pulsar::System::sInstance->IsContext(Pulsar::PULSAR_FASTBOX) ? Pulsar::DKWSETTING_ITEMBOX_FASTSPAWN : Pulsar::DKWSETTING_ITEMBOX_DEFAULT;
        isInstantBox = Pulsar::System::sInstance->IsContext(Pulsar::PULSAR_INSTANTBOX) ? Pulsar::DKWSETTING_ITEMBOX_INSTANTSPAWN : Pulsar::DKWSETTING_ITEMBOX_DEFAULT;
    }
    itembox->respawnTime = 150;
    itembox->isActive = 0;
    if (isFastBox == Pulsar::DKWSETTING_ITEMBOX_FASTSPAWN) {
        itembox->respawnTime = 75;
        itembox->isActive = 0;
    } else if (isInstantBox == Pulsar::DKWSETTING_ITEMBOX_INSTANTSPAWN) {
        itembox->respawnTime = 1;
        itembox->isActive = 0;
    }
}
kmCall(0x80828EDC, ItemBoxRespawn);


kmRuntimeUse(0x806C55AC);
kmRuntimeUse(0x8076D6EC);
kmRuntimeUse(0x8076DC24);
kmRuntimeUse(0x8076E944);
kmRuntimeUse(0x808274A8);
static void NoItemBox() {
  kmRuntimeWrite32A(0x806C55AC, 0x80030B80);
  kmRuntimeWrite32A(0x8076D6EC, 0x80030B80);
  kmRuntimeWrite32A(0x8076DC24, 0x80030B80);
  kmRuntimeWrite32A(0x8076E944, 0x80030B80);
  kmRuntimeWrite32A(0x808274A8, 0x80130B80);
  if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE) {
      if (System::sInstance->IsContext(Pulsar::PULSAR_DISABLEBOX)) {
          kmRuntimeWrite32A(0x806C55AC, 0x38000003);
          kmRuntimeWrite32A(0x8076D6EC, 0x38000003);
          kmRuntimeWrite32A(0x8076DC24, 0x38000003);
          kmRuntimeWrite32A(0x8076E944, 0x38000003);
          kmRuntimeWrite32A(0x808274A8, 0x38000003);
        }
    }
}
PageLoadHook NoItemBoxPatch(NoItemBox);

//Force 30 FPS [Vabold]
kmWrite32(0x80554224, 0x3C808000);
kmWrite32(0x80554228, 0x88841204);
kmWrite32(0x8055422C, 0x48000044);

void FPSPatch() {
  FPSPatchHook = 0x00;
  if (static_cast<Pulsar::DKWSettingChangeFPSMode>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC), Pulsar::MISC_CHANGEFPS)) == Pulsar::DKWSETTING_FPS_30) {
      FPSPatchHook = 0x00FF0100;
  }
}
static PageLoadHook PatchFPS(FPSPatch);

//Deflicker [Toadette Hack Fan, Optllizer]
void DeFlicker() {
Blurry = 0x41820040;
if (static_cast<Pulsar::DKWSettingDeflicker>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC), Pulsar::MISC_DEFLICKER)) == Pulsar::DKWSETTING_DEFLICKER_ENABLED) {
Blurry = 0x48000040;
}
}
static RaceLoadHook Deflick(DeFlicker);

//Remove Background Blur [WTP Team?]
void PatchVisuals(){
RemoveBloom = 0x03000000;
RemoveBackgroundBlur = 0x3f000000;
if (static_cast<Pulsar::DKWSettingBlur>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC), Pulsar::MISC_BLUR)) == Pulsar::DKWSETTING_BLUR_ENABLED) {
    RemoveBloom = 0x00;
    RemoveBackgroundBlur = 0x30000000;
  }
}
static PageLoadHook VisualHook(PatchVisuals);

asmFunc GetUltraUncut() {
  ASM(
loc_0x0:
lis       r12, 0x8000;
lbz       r12, 0x1203(r12);
cmpwi     r12, 0;
beq       end;
lbz       r3, 0x1C(r29);
cmplwi    r3, 0x1;
ble+      loc_0x10;
mr        r0, r30;

loc_0x10:
cmplw     r30, r0;
blr;

end:
cmplw     r30, r0;
blr;
  )
}
kmCall(0x8053511C, GetUltraUncut);

void UltraUncutPatch() {
UltraUncutHook = 0x00FF0100;
const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
const GameMode mode = scenario.settings.gamemode;

if(RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST || RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST || mode == MODE_VS_RACE || mode == MODE_TIME_TRIAL || mode == MODE_GHOST_RACE){
  if (System::sInstance->IsContext(Pulsar::PULSAR_ULTRAS) == Pulsar::DKWSETTING_ULTRAS_DISABLED) {
      UltraUncutHook = 0x00;
     }
}
}
static PageLoadHook PatchUltraUncut(UltraUncutPatch);

asmFunc MegaSizeModifier() {
    ASM(
        nofralloc;
        lfs       f0, 0x184(r4);
        lis       r5, 0x3FC0;
        stw       r5, 0x184(r3);
        lfs       f1, 0x184(r3);
        fmuls     f0, f1, f0;
        blr;)
}

kmRuntimeUse(0x805731CC); //Item Damage Type Modifier (Bob-Omb Explosion)
kmRuntimeUse(0x805731B4); //Item Damage Type Modifier (Blue Shell Explosion)
kmRuntimeUse(0x805731A8); //Item Damage Type Modifier (Red/Green Shell)
kmRuntimeUse(0x805808A4); //Item Damage Type Modifier (Shock)
kmRuntimeUse(0x808A4FA0); //FIB can block shells Part 1
kmRuntimeUse(0x808A4FA4); //FIB can block shells Part 2
kmRuntimeUse(0x808A4E18); //FIB can block shells Part 3
kmRuntimeUse(0x808A4E54); //FIB can block shells Part 4
kmRuntimeUse(0x808A5B74); //Blue Shell Explosion Size Effect Modifier
kmRuntimeUse(0x808A5C90); //Blue Shell Explosion Visual Size Modifier
kmRuntimeUse(0x809C2F90); //Growing Shells Part 1
kmRuntimeUse(0x809C3004); //Growing Shells Part 2
kmRuntimeUse(0x808B6864); //Better Steering Bullet Bill
kmRuntimeUse(0x808B682C); //360 Turning Bullet Bill Part 1
kmRuntimeUse(0x808B6834); //360 Turning Bullet Bill Part 2
kmRuntimeUse(0x8059D50A); //360 Turning Bullet Bill Part 3
kmRuntimeUse(0x80592128); //Mega Mushroom Size Modifier
kmRuntimeUse(0x808A5538); //Bob-omb Launch Modifier
kmRuntimeUse(0x8079CBEA); //Motion Sensor Bomb Part 1
kmRuntimeUse(0x8079BABE); //Motion Sensor Bomb Part 2
kmRuntimeUse(0x808A54CC); //Growing Bob-ombs
kmRuntimeUse(0x808B59C0); //Shock Speed Multiplier Modifier
kmRuntimeUse(0x808B5AB8); //Star Timer Modifier
kmRuntimeUse(0x807B1DD0); //Instant POW
kmRuntimeUse(0x808B5B28); //Mushroom Boost Modifier
static void MayhemGamemode() {
  kmRuntimeWrite32A(0x805731A8, 0x38600002); //Item Damage Type Modifier (Red/Green Shell)
  kmRuntimeWrite32A(0x805808A4, 0x3880000A); //Item Damage Type Modifier (Shock)
  kmRuntimeWrite32A(0x808A4FA0, 0x00000000); //FIB can block shells Part 1
  kmRuntimeWrite32A(0x808A4FA4, 0x00000000); //FIB can block shells Part 2
  kmRuntimeWrite32A(0x808A4E18, 0x00000000); //FIB can block shells Part 3
  kmRuntimeWrite32A(0x808A4E54, 0x00000000); //FIB can block shells Part 4
  kmRuntimeWrite32A(0x808A5B74, 0x41E80000); //Blue Shell Explosion Size Effect Modifier
  kmRuntimeWrite32A(0x808A5C90, 0x3F99999A); //Blue Shell Explosion Visual Size Modifier
  kmRuntimeWrite32A(0x809C2F90, 0x40066666); //Growing Shells Part 1
  kmRuntimeWrite32A(0x809C3004, 0x40066666); //Growing Shells Part 2
  kmRuntimeWrite32A(0x808B6864, 0xBC23D70A); //Better Steering Bullet Bill
  kmRuntimeWrite32A(0x808B682C, 0x3CF5C28F); //360 Turning Bullet Bill Part 1
  kmRuntimeWrite32A(0x808B6834, 0x3D4CCCCD); //360 Turning Bullet Bill Part 2
  kmRuntimeWrite16A(0x8059D50A, 0x0000981F); //360 Turning Bullet Bill Part 3
  kmRuntimeWrite32A(0x80592128, 0xC0040184); //Mega Mushroom Size Modifier
  kmRuntimeWrite32A(0x808A5538, 0x3F7FBE77); //Bob-omb Launch Modifier
  kmRuntimeWrite16A(0x8079CBEA, 0x00004182); //Motion Sensor Bomb Part 1
  kmRuntimeWrite16A(0x8079BABE, 0x001C2C1D); //Motion Sensor Bomb Part 2
  kmRuntimeWrite32A(0x808A54CC, 0x400CCCCD); //Growing Bob-ombs
  kmRuntimeWrite32A(0x808B59C0, 0x3F333333); //Shock Speed Multiplier Modifier
  kmRuntimeWrite16A(0x808B5AB8, 0x000001C2); //Star Timer Modifier
  kmRuntimeWrite32A(0x807B1DD0, 0x3800009F); //Instant POW
  kmRuntimeWrite16A(0x808B5B28, 0x0000005A); //Mushroom Boost Modifier
  if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL) {
      if (System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM)) {
          kmRuntimeWrite32A(0x805731A8, 0x38600006); //Item Damage Type Modifier (Red/Green Shell)
          kmRuntimeWrite32A(0x805731CC, 0x38600006); //Item Damage Type Modifier (Bob-omb Explosion)
          kmRuntimeWrite32A(0x805731B4, 0x38600006); //Item Damage Type Modifier (Blue Shell Explosion)
          kmRuntimeWrite32A(0x805808A4, 0x38800006); //Item Damage Type Modifier (Shock)
          kmRuntimeWrite32A(0x808A4FA0, 0x00000002); //FIB can block shells Part 1
          kmRuntimeWrite32A(0x808A4FA4, 0x00000002); //FIB can block shells Part 2
          kmRuntimeWrite32A(0x808A4E18, 0x00000002); //FIB can block shells Part 3
          kmRuntimeWrite32A(0x808A4E54, 0x00000002); //FIB can block shells Part 4
          kmRuntimeWrite32A(0x808A5B74, 0x42960000); //Blue Shell Explosion Size Effect Modifier
          kmRuntimeWrite32A(0x808A5C90, 0x40900000); //Blue Shell Explosion Visual Size Modifier
          kmRuntimeWrite32A(0x809C2F90, 0x42066666); //Growing Shells Part 1
          kmRuntimeWrite32A(0x809C3004, 0x42066666); //Growing Shells Part 2
          kmRuntimeWrite32A(0x808B6864, 0xBD600000); //Better Steering Bullet Bill
          kmRuntimeWrite32A(0x808B682C, 0x30000000); //360 Turning Bullet Bill Part 1
          kmRuntimeWrite32A(0x808B6834, 0x30000000); //360 Turning Bullet Bill Part 2
          kmRuntimeWrite16A(0x8059D50A, 0x00000000); //360 Turning Bullet Bill Part 3
          kmRuntimeCallA(0x80592128, MegaSizeModifier); //Mega Mushroom Size Modifier
          kmRuntimeWrite32A(0x808A5538, 0x3F830000); //Bob-omb Launch Modifier
          kmRuntimeWrite16A(0x8079CBEA, 0x00000FFF); //Motion Sensor Bomb Part 1
          kmRuntimeWrite16A(0x8079BABE, 0x00000FFF); //Motion Sensor Bomb Part 2
          kmRuntimeWrite32A(0x808A54CC, 0x41000000); //Growing Bob-ombs
          kmRuntimeWrite32A(0x808B59C0, 0x3E800000); //Shock Speed Multiplier Modifier
          kmRuntimeWrite16A(0x808B5AB8, 0x00000384); //Star Timer Modifier
          kmRuntimeWrite32A(0x807B1DD0, 0x38000037); //Instant POW
          kmRuntimeWrite16A(0x808B5B28, 0x000000B4); //Mushroom Boost Modifier
        }
    }
}
PageLoadHook MayhemGamemodePatch(MayhemGamemode);

asmFunc ItemLimitModifier() {
    ASM(
        nofralloc;
        li        r19, 0x10;
        stw       r19, 0x4(r4);
        blr;)
}

asmFunc StarAnimationModifier() {
    ASM(
        nofralloc;
        lhz       r0, 0xF6(r31);
        lwz       r12, 0x0(r31);
        lwz       r12, 0x4(r12);
        lwz       r12, 0x8(r12);
        andis.    r12, r12, 0x8000;
        beq-      loc_0x20;
        li        r0, 0x8;
        sth       r0, 0xF6(r31);

        loc_0x20:
        blr;)
}

kmRuntimeUse(0x805A228C); //Allow Looking Backwards During Respawn
kmRuntimeUse(0x807BB764); //Allow Mega in a Mega
kmRuntimeUse(0x8082AC00); //Mega Flips Cars
kmRuntimeUse(0x807AFAA8); //Impervious Thunder Cloud v2
kmRuntimeUse(0x805731CC); //Item Damage Type Modifier (Bob-Omb Explosion)
kmRuntimeUse(0x805731B4); //Item Damage Type Modifier (Blue Shell Explosion)
kmRuntimeUse(0x80790E94); //Max Item Limit Modifier
kmRuntimeUse(0x807CD2DC); //MKDS Star Character Animation
static void Mayhem() {
  kmRuntimeWrite32A(0x805A228C, 0x40820038);
  kmRuntimeWrite32A(0x807BB764, 0x4182000C);
  kmRuntimeWrite32A(0x8082AC00, 0x3B800002);
  kmRuntimeWrite32A(0x807AFAA8, 0x801D0240);
  kmRuntimeWrite32A(0x805731CC, 0x38600007);
  kmRuntimeWrite32A(0x805731B4, 0x38600007);
  kmRuntimeWrite32A(0x80790E94, 0x82640004);
  kmRuntimeWrite32A(0x807CD2DC, 0xA01F00F6);
  if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL) {
      if (System::sInstance->IsContext(Pulsar::PULSAR_MAYHEM)) {
          kmRuntimeWrite32A(0x805A228C, 0x60000000);
          kmRuntimeWrite32A(0x807BB764, 0x60000000);
          kmRuntimeWrite32A(0x8082AC00, 0x3B800001);
          kmRuntimeWrite32A(0x807AFAA8, 0x48000038);
          kmRuntimeWrite32A(0x805731CC, 0x38600002);
          kmRuntimeWrite32A(0x805731B4, 0x38600004);
          kmRuntimeCallA(0x80790E94, ItemLimitModifier);
          kmRuntimeCallA(0x807CD2DC, StarAnimationModifier);
        }
    }
}
PageLoadHook MayhemPatch(Mayhem);

kmRuntimeUse(0x80798004);
static void TCAutostart() {
  kmRuntimeWrite32A(0x80798004, 0x40820040);
  if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE) {
      if (System::sInstance->IsContext(Pulsar::PULSAR_TCTOGGLE)) {
          kmRuntimeWrite32A(0x80798004, 0x48000040);
    }
  }
}
PageLoadHook TCPatch(TCAutostart);

kmRuntimeUse(0x807C897C);
static void LumaToggle() {
  kmRuntimeWrite16A(0x807C897C, 0x00004800);
    if (static_cast<Pulsar::DKWSettingLuma>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC3), Pulsar::MISC_LUMA)) == Pulsar::DKWSETTING_LUMA_ENABLED) {
        kmRuntimeWrite16A(0x807C897C, 0x00004082);
    }
}
PageLoadHook LumaPatch(LumaToggle);

asmFunc MegaFOV() {
    ASM(
        nofralloc;
        lwz       r4, 0x0(r28);
        lwz       r29, 0x24(r4);
        cmpwi     r29, 0;
        beq-      loc_0x28;
        lwz       r3, 0x4(r4);
        lwz       r3, 0xC(r3);
        rlwinm.   r3,r3,0,16,16;
        beq-      loc_0x28;
        lis       r0, 0x41F0;
        stw       r0, 0x120(r29);

        loc_0x28:
        blr;)
}

kmRuntimeUse(0x805793AC);
static void EnhancedMegaFOV() {
kmRuntimeWrite32A(0x805793AC, 0x809C0000);
  if (static_cast<Pulsar::DKWSettingBetterMegaFOV>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC2), Pulsar::MISC_MEGAFOV)) == Pulsar::DKWSETTING_MEGAFOV_ENABLED) {
          kmRuntimeCallA(0x805793AC, MegaFOV);
  }
}
PageLoadHook MegaFOVPatch(EnhancedMegaFOV);     

} // namespace DKW