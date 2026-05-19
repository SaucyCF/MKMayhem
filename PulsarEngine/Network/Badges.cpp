#include <DKW.hpp>
#include <MarioKartWii/Race/Racedata.hpp>
#include <MarioKartWii/Race/Raceinfo/Raceinfo.hpp>
#include <MarioKartWii/RKSYS/LicenseMgr.hpp>
#include <MarioKartWii/System/Identifiers.hpp>
#include <MarioKartWii/RKSYS/RKSYSMgr.hpp>
#include <MarioKartWii/GlobalFunctions.hpp>
#include <MarioKartWii/System/Rating.hpp>
#include <MarioKartWii/RKNet/USER.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <runtimeWrite.hpp>
#include <core/rvl/OS/OS.hpp>

namespace Pulsar {
namespace Badges {

// Friend Code override list: players in this list get the priority badge (value 10)
static const u64 SAUCY_FC[] = {
    500000000ULL,
    7008ULL,
};

// Friend Code override list: players in this list get the priority badge (value 9)
static const u64 SCOTT_FC[] = {
    442983893426ULL,
    323122547350ULL,
};

// Friend Code override list: players in this list get the priority badge (value 8)
static const u64 AGENT_FC[] = {
    262595141029ULL,
    331314639143ULL,
};

// Friend Code override list: players in this list get the priority badge (value 7)
static const u64 TWITCHSUB_FC[] = {
    0ULL,
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

static bool IsTwitchSubsFC(u64 fc) {
    if (fc == 0) return false;
    for (size_t i = 0; TWITCHSUB_FC[i] != 0ULL; ++i) {
        if (TWITCHSUB_FC[i] == fc) return true;
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
        } if (IsTwitchSubsFC(myFc)) {
            kmRuntimeWrite32A(0x806436a0, 0x38600007);  // li r3,7
        }
    }
}
SectionLoadHook HookRankIcon(DisplayOnlineRanking);

// Badge Anticheat - verify that the badge assignment is legitimate
// If a player has a badge but their FC isn't in any authorized list, fatal error
static void VerifyBadgeIntegrity() {
    // Only check when online
    RKNet::Controller* rkNetController = RKNet::Controller::sInstance;
    if (rkNetController == nullptr || rkNetController->connectionState == 0) return;
    
    // Read the current runtime value at the badge address
    u32 currentBadgeValue = *(volatile u32*)0x806436a0;

    if (currentBadgeValue != 0x3860000A) return;
    
    // Get the player's FC
    if (RKNet::USERHandler::sInstance == nullptr || !RKNet::USERHandler::sInstance->isInitialized) {
        return;
    }
    
    const u64 myFc = RKNet::USERHandler::sInstance->toSendPacket.fc;
    
    // Check if FC is in any valid list
    bool isValidFC = IsSaucysFC(myFc) || IsScottsFC(myFc) || IsAgentsFC(myFc) || IsTwitchSubsFC(myFc);
    
    if (!isValidFC) {
        // Player has a badge but isn't authorized - cheating detected
        Pulsar::Debug::FatalError("Badge verification failed.");
    }
}
RaceLoadHook BadgeAntiCheat(VerifyBadgeIntegrity);

}  // namespace Badges
}  // namespace Pulsar
