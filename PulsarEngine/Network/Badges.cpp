#include <DKW.hpp>
#include <MarioKartWii/Race/Racedata.hpp>
#include <MarioKartWii/Race/Raceinfo/Raceinfo.hpp>
#include <MarioKartWii/RKSYS/LicenseMgr.hpp>
#include <MarioKartWii/System/Identifiers.hpp>
#include <MarioKartWii/RKSYS/RKSYSMgr.hpp>
#include <MarioKartWii/GlobalFunctions.hpp>
#include <MarioKartWii/System/Rating.hpp>
#include <MarioKartWii/RKNet/USER.hpp>
#include <runtimeWrite.hpp>
#include <core/rvl/OS/OS.hpp>

namespace Pulsar {
namespace Badges {

// Friend Code override list: players in this list get the priority badge (value 10)
static const u64 SAUCY_FC[] = {
    77787777770ULL,
};

// Friend Code override list: players in this list get the priority badge (value 9)
static const u64 SCOTT_FC[] = {
    442983893426ULL,
};

// Friend Code override list: players in this list get the priority badge (value 8)
static const u64 AGENT_FC[] = {
    262595141029ULL,
    331314639143ULL,
};

static bool IsSaucysFC(u64 fc) {
    if (fc == 0) return false;
    for (size_t i = 0; SAUCY_FC[i] != 0ULL; ++i) {
        if (SAUCY_FC[i] == fc) return true;
    }
    return false;
}

static bool IsScottsFC(u64 fc) {
    if (fc == 0) return false;
    for (size_t i = 0; SCOTT_FC[i] != 0ULL; ++i) {
        if (SCOTT_FC[i] == fc) return true;
    }
    return false;
}

static bool IsAgentsFC(u64 fc) {
    if (fc == 0) return false;
    for (size_t i = 0; AGENT_FC[i] != 0ULL; ++i) {
        if (AGENT_FC[i] == fc) return true;
    }
    return false;
}

// Address found by B_squo, developed by ZPL
kmRuntimeUse(0x806436a0);
static void DisplayOnlineRanking() {
    // Default to rank 0
    kmRuntimeWrite32A(0x806436a0, 0x38600000);  // li r3,0

    // Priority badge override for specific friend codes, takes precedence over any ranking
    // Source for friend code: RKNet::USERHandler::toSendPacket.fc (local player's FC)
    // Provenance: structure defined in GameSource/MarioKartWii/RKNet/USER.hpp
    if (RKNet::USERHandler::sInstance != nullptr && RKNet::USERHandler::sInstance->isInitialized) {
        const u64 myFc = RKNet::USERHandler::sInstance->toSendPacket.fc;
        if (IsSaucysFC(myFc)) {
            kmRuntimeWrite32A(0x806436a0, 0x3860000A);  // li r3,10
        } if (IsScottsFC(myFc)) {
            kmRuntimeWrite32A(0x806436a0, 0x38600009);  // li r3,9
        } if (IsAgentsFC(myFc)) {
            kmRuntimeWrite32A(0x806436a0, 0x38600008);  // li r3,8
        }
    }
}
static SectionLoadHook HookRankIcon(DisplayOnlineRanking);

}  // namespace Badges
}  // namespace Pulsar