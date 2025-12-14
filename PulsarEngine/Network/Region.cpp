#include <kamek.hpp>
#include <core/rvl/DWC/DWC.hpp>
#include <PulsarSystem.hpp>
#include <Settings/Settings.hpp>
#include <Network/WiiLink.hpp>
#include <Network/Network.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>

namespace Pulsar {
namespace Network {

// Region Patch (Leseratte)
static void PatchRegionNumber() {
const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
const GameMode mode = scenario.settings.gamemode;
    if (!System::sInstance->IsContext(Pulsar::PULSAR_MODE_ITEMRAIN) && !System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM) && mode == MODE_PUBLIC_VS) {
        System::sInstance->netMgr.region = 0x4D;
    } else if (System::sInstance->IsContext(Pulsar::PULSAR_MODE_ITEMRAIN) && mode == MODE_PUBLIC_VS) {
        System::sInstance->netMgr.region = 0x4E;
    } else if (System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM) && mode == MODE_PUBLIC_VS) {
        System::sInstance->netMgr.region = 0x4F;
    } else if (static_cast<Pulsar::DKWSettingWWGamemode>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC2), Pulsar::WW_GAMEMODE)) == Pulsar::DKWSETTING_WWGAMEMODE_MKDS) {
        System::sInstance->netMgr.region = 0x4D;
    } else if (static_cast<Pulsar::DKWSettingWWGamemode>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC2), Pulsar::WW_GAMEMODE)) == Pulsar::DKWSETTING_WWGAMEMODE_ITEMRAIN) {
        System::sInstance->netMgr.region = 0x4E;
    } else if (static_cast<Pulsar::DKWSettingWWGamemode>(Pulsar::Settings::Mgr::Get().GetSettingValue(static_cast<Pulsar::Settings::Type>(Pulsar::Settings::SETTINGSTYPE_MISC2), Pulsar::WW_GAMEMODE)) == Pulsar::DKWSETTING_WWGAMEMODE_MAYHEM) {
        System::sInstance->netMgr.region = 0x4F;
    } else {
        System::sInstance->netMgr.region = 0x4D;
    }
}
static PageLoadHook RegionNumberPatch(PatchRegionNumber);

static void PatchLoginRegion() {
    WWFC_CUSTOM_REGION = System::sInstance->netMgr.region;
    char path[0x9];
    snprintf(path, 0x9, "%08d", System::sInstance->netMgr.region + 100000);
    for (int i = 0; i < 8; ++i) {
        DWC::loginRegion[i] = path[i];
    }
}
BootHook LoginRegion(PatchLoginRegion, 2);

// PatchRegion(0x8065920c);
// PatchRegion(0x80659260);
// PatchRegion(0x80659724);
// PatchRegion(0x80659778);

int PatchRegion(char* path, u32 len, const char* fmt, const char* mode) {
    const Info& info = System::sInstance->GetInfo();
    return snprintf(path, len, fmt, mode, System::sInstance->netMgr.region);
}
kmCall(0x8065921c, PatchRegion);
kmCall(0x80659270, PatchRegion);
kmCall(0x80659734, PatchRegion);
kmCall(0x80659788, PatchRegion);

// kmWrite32(0x8065a038, 0x7C050378);
// kmWrite32(0x8065a084, 0x7C050378);
static int GetFriendsSearchType(int curType, u32 regionId) {
    register u8 friendRegionId;
    asm(mr friendRegionId, r0;);
    if ((System::sInstance->netMgr.region == 0x4D || System::sInstance->netMgr.region == 0x4E || System::sInstance->netMgr.region == 0x4F) || (friendRegionId == 0x4D || friendRegionId == 0x4E || friendRegionId == 0x4F)) {
        if (curType == 7) return 6;
        return 9;
    }
    if (System::sInstance->netMgr.region != friendRegionId) return curType;
    if (curType == 7) return 6;
    return 9;
}
kmBranch(0x8065a03c, GetFriendsSearchType);
kmBranch(0x8065a088, GetFriendsSearchType);

static u32 PatchRKNetControllerRegion() {
    return System::sInstance->netMgr.region;
}
kmCall(0x80653640, PatchRKNetControllerRegion);
kmWrite32(0x80653644, 0x7c651b78);
kmCall(0x806536ac, PatchRKNetControllerRegion);  // for battle
kmWrite32(0x806536b0, 0x7c661b78);

// kmCall(0x80653700, PatchRKNetControllerRegion); this is for battle, right now it'll store 2 (if pal)/FF

// kmWrite32(0x8065A034, 0x3880008E);
// kmWrite32(0x8065A080, 0x3880008E);

}  // namespace Network
}  // namespace Pulsar