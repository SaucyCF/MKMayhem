#include <kamek.hpp>
#include <Settings/SettingsParam.hpp>
#include <MarioKartWii/Race/Racedata.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <Settings/UI/SettingsPanel.hpp>
#include <Settings/Settings.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <PulsarSystem.hpp>
#include <DKW.hpp>
#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <Network/SHA256.hpp>

namespace DKW {
    Pulsar::System *System::Create() {
        return new System();
}
Pulsar::System::Inherit CreateDKW(System::Create);

bool System::Is50cc() {
  const Pulsar::Settings::Mgr& settings = Pulsar::Settings::Mgr::Get();
  return Racedata::sInstance->racesScenario.settings.engineClass == CC_50 && settings.GetUserSettingValue(Pulsar::Settings::SETTINGSTYPE_DKW1, Pulsar::SETTINGDKW_CUSTOMCC) == Pulsar::DKWSETTING_CUSTOMCC_50;
}
bool System::Is100cc() {
  const Pulsar::Settings::Mgr& settings = Pulsar::Settings::Mgr::Get();
  return Racedata::sInstance->racesScenario.settings.engineClass == CC_50 && settings.GetUserSettingValue(Pulsar::Settings::SETTINGSTYPE_DKW1, Pulsar::SETTINGDKW_CUSTOMCC) == Pulsar::DKWSETTING_CUSTOMCC_100;
}
bool System::Is400cc() {
  const Pulsar::Settings::Mgr& settings = Pulsar::Settings::Mgr::Get();
  return Racedata::sInstance->racesScenario.settings.engineClass == CC_50 && settings.GetUserSettingValue(Pulsar::Settings::SETTINGSTYPE_DKW1, Pulsar::SETTINGDKW_CUSTOMCC) == Pulsar::DKWSETTING_CUSTOMCC_400;
}
bool System::Is99999cc() {
  const Pulsar::Settings::Mgr& settings = Pulsar::Settings::Mgr::Get();
  return Racedata::sInstance->racesScenario.settings.engineClass == CC_50 && settings.GetUserSettingValue(Pulsar::Settings::SETTINGSTYPE_DKW1, Pulsar::SETTINGDKW_CUSTOMCC) == Pulsar::DKWSETTING_CUSTOMCC_99999;
}

System::WeightClass System::GetWeightClass(const CharacterId id){
    switch (id)
    {
        case BABY_MARIO:
        case BABY_LUIGI:
        case BABY_PEACH:
        case BABY_DAISY:
        case TOAD:
        case TOADETTE:
        case KOOPA_TROOPA:
        case DRY_BONES:
            return LIGHTWEIGHT;
        case MARIO:
        case LUIGI:
        case PEACH:
        case DAISY:
        case YOSHI:
        case BIRDO:
        case DIDDY_KONG:
        case BOWSER_JR:
            return MEDIUMWEIGHT;
        case WARIO:
        case WALUIGI:
        case DONKEY_KONG:
        case BOWSER:
        case KING_BOO:
        case ROSALINA:
        case FUNKY_KONG:
        case DRY_BOWSER:
            return HEAVYWEIGHT;
        default:
            return MIIS;
    }
}

//Force 30 FPS [Vabold]
kmWrite32(0x80554224, 0x3C808000);
kmWrite32(0x80554228, 0x88841204);
kmWrite32(0x8055422C, 0x48000044);

void FPSPatch() {
  FPSPatchHook = 0x00;
  if (static_cast<Pulsar::DKWSettingChangeFPSMode>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_CHANGEFPS)) == Pulsar::DKWSETTING_FPS_30) {
      FPSPatchHook = 0x00FF0100;
  }
}
static PageLoadHook PatchFPS(FPSPatch);

//Deflicker [Toadette Hack Fan, Optllizer]
void DeFlicker() {
Blurry = 0x41820040;
if (static_cast<Pulsar::DKWSettingFlickerBlur>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_FLICKERBLUR)) == Pulsar::DKWSETTING_FLICKERBLUR_ENABLED) {
Blurry = 0x48000040;
}
}
static RaceLoadHook Deflick(DeFlicker);

//Remove Background Blur [WTP Team?]
void PatchVisuals(){
RemoveBloom = 0x03000000;
RemoveBackgroundBlur = 0x3f000000;
if (static_cast<Pulsar::DKWSettingFlickerBlur>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_FLICKERBLUR)) == Pulsar::DKWSETTING_FLICKERBLUR_ENABLED) {
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

void ColourChangeToggle(){
    U8_MENUSINGLE1 = 0x65;
    U8_MENUMULTI1 = 0x69;
    U8_RACE_MENUS1 = 0x65;
    U8_COMMONFILE = 0x6E;
    U8_RED1= 0xFF;
    U8_GREEN1= 0xFF;
    U8_BLUE1= 0x00;
    U8_ALPHA1= 0xFF;
    U8_RED2= 0xD2;
    U8_GREEN2= 0xAA;
    U8_BLUE2= 0x00;
    U8_ALPHA2= 0xFF;
    U8_RED3= 0xD2;
    U8_GREEN3= 0xAA;
    U8_BLUE3= 0x00;
    U8_ALPHA3= 0x46;
    if (static_cast<Pulsar::DKWSettingUIColour>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_UICOLOUR)) == Pulsar::DKWSETTING_UICOLOUR_RED){
        U8_MENUSINGLE1 = 0x67;
        U8_MENUMULTI1 = 0x67;
        U8_RACE_MENUS1 = 0x67;
        U8_COMMONFILE = 0x67;
        U8_RED1= 0xFF;
        U8_GREEN1= 0x00;
        U8_BLUE1= 0x00;
        U8_ALPHA1= 0xFF;
        U8_RED2= 0xFF;
        U8_GREEN2= 0x00;
        U8_BLUE2= 0x00;
        U8_ALPHA2= 0xFF;
        U8_RED3= 0xFF;
        U8_GREEN3= 0x00;
        U8_BLUE3= 0x00;
        U8_ALPHA3= 0x46;
    }
    else if (static_cast<Pulsar::DKWSettingUIColour>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_UICOLOUR)) == Pulsar::DKWSETTING_UICOLOUR_ORANGE){
        U8_MENUSINGLE1 = 0x61;
        U8_MENUMULTI1 = 0x61;
        U8_RACE_MENUS1 = 0x61;
        U8_COMMONFILE = 0x61;
        U8_RED1= 0xFF;
        U8_GREEN1= 0x7D;
        U8_BLUE1= 0x00;
        U8_ALPHA1= 0xFF;
        U8_RED2= 0xFF;
        U8_GREEN2= 0x7D;
        U8_BLUE2= 0x00;
        U8_ALPHA2= 0xFF;
        U8_RED3= 0xFF;
        U8_GREEN3= 0x7D;
        U8_BLUE3= 0x00;
        U8_ALPHA3= 0x46;
    }
    else if (static_cast<Pulsar::DKWSettingUIColour>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_UICOLOUR)) == Pulsar::DKWSETTING_UICOLOUR_GREEN){
        U8_MENUSINGLE1 = 0x62;
        U8_MENUMULTI1 = 0x62;
        U8_RACE_MENUS1 = 0x62;
        U8_COMMONFILE = 0x62;
        U8_RED1= 0x00;
        U8_GREEN1= 0xFF;
        U8_BLUE1= 0x00;
        U8_ALPHA1= 0xFF;
        U8_RED2= 0x00;
        U8_GREEN2= 0xFF;
        U8_BLUE2= 0x00;
        U8_ALPHA2= 0xFF;
        U8_RED3= 0x00;
        U8_GREEN3= 0xFF;
        U8_BLUE3= 0x00;
        U8_ALPHA3= 0x46;
    }
    else if (static_cast<Pulsar::DKWSettingUIColour>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_UICOLOUR)) == Pulsar::DKWSETTING_UICOLOUR_BLUE){
        U8_MENUSINGLE1 = 0x63;
        U8_MENUMULTI1 = 0x63;
        U8_RACE_MENUS1 = 0x63;
        U8_COMMONFILE = 0x63;
        U8_RED1= 0x00;
        U8_GREEN1= 0x00;
        U8_BLUE1= 0xFF;
        U8_ALPHA1= 0xFF;
        U8_RED2= 0x00;
        U8_GREEN2= 0x00;
        U8_BLUE2= 0xFF;
        U8_ALPHA2= 0xFF;
        U8_RED3= 0x00;
        U8_GREEN3= 0x00;
        U8_BLUE3= 0xFF;
        U8_ALPHA3= 0x46;
    }
    else if (static_cast<Pulsar::DKWSettingUIColour>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_UICOLOUR)) == Pulsar::DKWSETTING_UICOLOUR_PURPLE){
        U8_MENUSINGLE1 = 0x64;
        U8_MENUMULTI1 = 0x64;
        U8_RACE_MENUS1 = 0x64;
        U8_COMMONFILE = 0x64;
        U8_RED1= 0x7D;
        U8_GREEN1= 0x00;
        U8_BLUE1= 0xFF;
        U8_ALPHA1= 0xFF;
        U8_RED2= 0x7D;
        U8_GREEN2= 0x00;
        U8_BLUE2= 0xFF;
        U8_ALPHA2= 0xFF;
        U8_RED3= 0x7D;
        U8_GREEN3= 0x00;
        U8_BLUE3= 0xFF;
        U8_ALPHA3= 0x46;
    }
    else if (static_cast<Pulsar::DKWSettingUIColour>(Pulsar::Settings::Mgr::Get().GetUserSettingValue(static_cast<Pulsar::Settings::UserType>(Pulsar::Settings::SETTINGSTYPE_DKW5), Pulsar::SETTINGDKW_UICOLOUR)) == Pulsar::DKWSETTING_UICOLOUR_PINK){
        U8_MENUSINGLE1 = 0x66;
        U8_MENUMULTI1 = 0x66;
        U8_RACE_MENUS1 = 0x66;
        U8_COMMONFILE = 0x66;
        U8_RED1= 0xFF;
        U8_GREEN1= 0x7D;
        U8_BLUE1= 0xFF;
        U8_ALPHA1= 0xFF;
        U8_RED2= 0xFF;
        U8_GREEN2= 0x7D;
        U8_BLUE2= 0xFF;
        U8_ALPHA2= 0xFF;
        U8_RED3= 0xFF;
        U8_GREEN3= 0x7D;
        U8_BLUE3= 0xFF;
        U8_ALPHA3= 0x46;
    }
    U8_MENUSINGLE2 = U8_MENUSINGLE1;
    U8_MENUMULTI2 = U8_MENUMULTI1;
    U8_RACE_MENUS2 = U8_RACE_MENUS1;
    U8_MENUSINGLE3 = U8_MENUSINGLE1;
    U8_MENUMULTI3 = U8_MENUMULTI1;
    U8_RACE_MENUS3 = U8_RACE_MENUS1;
    U8_MENUSINGLE4 = U8_MENUSINGLE1;
    U8_MENUMULTI4 = U8_MENUMULTI1;
    U8_RED4 = U8_RED1;
    U8_GREEN4 = U8_GREEN1;
    U8_BLUE4 = U8_BLUE1;
    U8_ALPHA4 = U8_ALPHA1;
    U8_MENUSINGLE5 = U8_MENUSINGLE1;
    U8_RED5 = U8_RED2;
    U8_GREEN5 = U8_GREEN2;
    U8_BLUE5 = U8_BLUE2;
    U8_ALPHA5 = U8_ALPHA2;
    U8_RED6 = U8_RED3;
    U8_GREEN6 = U8_GREEN3;
    U8_BLUE6 = U8_BLUE3;
    U8_ALPHA6 = U8_ALPHA3;
}
static PageLoadHook PatchColourChange(ColourChangeToggle);

void SHA256(const void* data, unsigned long len, unsigned char* out) {
        SHA256Context ctx;
        SHA256Init(&ctx);
        SHA256Update(&ctx, data, len);
        u8* digest = SHA256Final(&ctx);
        for (int i = 0; i < SHA256_DIGEST_SIZE; ++i) {
            out[i] = digest[i];
        }
    }
} // namespace DKW