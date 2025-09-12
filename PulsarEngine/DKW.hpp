#include <kamek.hpp>
#include <PulsarSystem.hpp>
#include <Settings/Settings.hpp>
#include <MarioKartWii/GlobalFunctions.hpp>
#include <MarioKartWii/System/Identifiers.hpp>
#include <MarioKartWii/Race/RaceData.hpp>

//All Code Credits go to the WTP Team unless otherwise mentioned.

extern u16 MKMSnaking;
extern u16 MKMRanked;
extern u16 MKMItemRain;
extern u16 MKMayhem;
extern u16 MKMHybrid;
extern u16 MKMItemStatus;
extern u16 MKMBattleRoyale;
extern u16 MKMItemBox;
extern u16 MKMMegaFOV;
extern u16 MKMRemoveLuma;
extern u16 MKMStarDance;
extern u16 MKMItemBox;
extern u16 MKMTCToggle;
extern u8 U8_MENUSINGLE1;
extern u8 U8_MENUSINGLE2;
extern u8 U8_MENUSINGLE3;
extern u8 U8_MENUSINGLE4;
extern u8 U8_MENUSINGLE5;
extern u8 U8_MENUMULTI1;
extern u8 U8_MENUMULTI2;
extern u8 U8_MENUMULTI3;
extern u8 U8_MENUMULTI4;
extern u8 U8_RACE_MENUS1;
extern u8 U8_RACE_MENUS2;
extern u8 U8_RACE_MENUS3;
extern u8 U8_COMMONFILE;
extern u8 U8_RED1;
extern u8 U8_GREEN1;
extern u8 U8_BLUE1;
extern u8 U8_ALPHA1;
extern u8 U8_RED2;
extern u8 U8_GREEN2;
extern u8 U8_BLUE2;
extern u8 U8_ALPHA2;
extern u8 U8_RED3;
extern u8 U8_GREEN3;
extern u8 U8_BLUE3;
extern u8 U8_ALPHA3;
extern u8 U8_RED4;
extern u8 U8_GREEN4;
extern u8 U8_BLUE4;
extern u8 U8_ALPHA4;
extern u8 U8_RED5;
extern u8 U8_GREEN5;
extern u8 U8_BLUE5;
extern u8 U8_ALPHA5;
extern u8 U8_RED6;
extern u8 U8_GREEN6;
extern u8 U8_BLUE6;
extern u8 U8_ALPHA6;
extern u32 FPSPatchHook;
extern u32 Blurry;
extern u32 RemoveBloom;
extern u32 RemoveBackgroundBlur;
extern u32 ItemRainOnlineFixHook;
extern u32 UltraUncutHook;
extern u32 DolphinCheat;
extern u32 AntiCheat1;
extern u32 AntiCheat2;
extern u32 AntiCheat3;
extern u32 AntiCheat4;
extern u32 AntiCheat5;
extern u32 AntiCheat6;
extern u32 AntiCheat7;
extern u32 GeckoLoader1;

namespace DKW {
class System : public Pulsar::System {
public:
    static bool Is50cc();
    static bool Is100cc();
    static bool Is400cc();
    static bool Is99999cc();

    enum WeightClass{
        LIGHTWEIGHT,
        MEDIUMWEIGHT,
        HEAVYWEIGHT,
        MIIS,
        ALLWEIGHT
    };

    enum CharButtonId{
        BUTTON_BABY_MARIO,
        BUTTON_BABY_LUIGI,
        BUTTON_TOAD,
        BUTTON_TOADETTE,
        BUTTON_BABY_PEACH,
        BUTTON_KOOPA_TROOPA,
        BUTTON_BABY_DAISY,
        BUTTON_DRY_BONES,
        BUTTON_MARIO,
        BUTTON_LUIGI,
        BUTTON_YOSHI,
        BUTTON_BIRDO,
        BUTTON_PEACH,
        BUTTON_DIDDY_KONG,
        BUTTON_DAISY,
        BUTTON_BOWSER_JR,
        BUTTON_WARIO,
        BUTTON_WALUIGI,
        BUTTON_KING_BOO,
        BUTTON_ROSALINA,
        BUTTON_DONKEY_KONG,
        BUTTON_FUNKY_KONG,
        BUTTON_BOWSER,
        BUTTON_DRY_BOWSER,
        BUTTON_MII_A,
        BUTTON_MII_B
    };

    WeightClass weight;
    static Pulsar::System *Create(); //My Create function, needs to return Pulsar
    static WeightClass GetWeightClass(CharacterId);
    };
}