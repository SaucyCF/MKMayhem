#include <kamek.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <PulsarSystem.hpp>
#include <Gamemodes/KO/KOMgr.hpp>
#include <Gamemodes/KO/KOSuperRarePower.hpp>

namespace Pulsar {
namespace KO {

// Track which players have Super Rare Power for the current race (bitmask)
static u16 sSuperRarePowerMask = 0;

// Track which players just won Super Rare Power (for results screen display)
static u16 sJustWonSuperRarePowerMask = 0;

// Track which players will have Super Rare Power for the NEXT race
static u16 sNextRaceSuperRarePowerMask = 0;

// Pre-rolled mask - host rolls during race, synced to clients, applied in ProcessKOs
static u16 sPreRolledSuperRarePowerMask = 0;

// Flag to track if we've already rolled this race
static bool sHasPreRolledThisRace = false;

// Simple random number generator using game's random
static s32 GetRandomNumber(int max) {
    // Use the game's Raceinfo random if available, otherwise use a simple approach
    Raceinfo* raceinfo = Raceinfo::sInstance;
    if (raceinfo != nullptr && raceinfo->random != nullptr) {
        return raceinfo->random->NextLimited(max);
    }
    // Fallback - shouldn't happen during race
    return 0;
}

// Check if Super Rare Power feature is enabled (only in KO mode with superpowers)
static inline bool IsSuperRarePowerEnabled() {
    const System* system = System::sInstance;
    return system != nullptr && system->IsContext(PULSAR_MODE_KO) && system->IsContext(PULSAR_SUPERPOWERS);
}

// Check if we are the host
static inline bool IsHost() {
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    if (controller == nullptr) return true; // Assume host if no network
    const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];
    return sub.localAid == sub.hostAid;
}

// Pre-roll Super Rare Power during race (host only)
// Called periodically during race to ensure dice are rolled before results
void PreRollSuperRarePower() {
    if (!IsSuperRarePowerEnabled()) return;
    if (sHasPreRolledThisRace) return;  // Only roll once per race
    if (!IsHost()) return;  // Only host rolls
    
    const System* system = System::sInstance;
    if (system == nullptr || system->koMgr == nullptr) return;
    
    const Mgr* koMgr = system->koMgr;
    const u8 playerCount = system->nonTTGhostPlayersCount;
    
    // Roll for each player
    for (u8 playerId = 0; playerId < playerCount && playerId < 12; ++playerId) {
        // Don't give to KO'd or disconnected players
        if (koMgr->IsKOdPlayerId(playerId) || koMgr->IsDisconnectedPlayerId(playerId)) {
            continue;
        }
        
        // Roll the dice - 1 in 120 chance
        const s32 roll = GetRandomNumber(120);
        if (roll == 0) {
            sPreRolledSuperRarePowerMask |= (1 << playerId);
        }
    }
    
    sHasPreRolledThisRace = true;
}

// Apply the pre-rolled results to the display masks (called in ProcessKOs)
void ApplyPreRolledSuperRarePower() {
    if (!IsSuperRarePowerEnabled()) return;
    
    // Apply the pre-rolled mask (from host or synced from host)
    sJustWonSuperRarePowerMask = sPreRolledSuperRarePowerMask;
    sNextRaceSuperRarePowerMask = sPreRolledSuperRarePowerMask;
}

// Reset Super Rare Power at the start of each race
static void ResetSuperRarePowerForRace() {
    if (!IsSuperRarePowerEnabled()) {
        sSuperRarePowerMask = 0;
        sJustWonSuperRarePowerMask = 0;
        sPreRolledSuperRarePowerMask = 0;
        sHasPreRolledThisRace = false;
        return;
    }
    
    // Apply the next race power (from previous race's roll)
    sSuperRarePowerMask = sNextRaceSuperRarePowerMask;
    
    // Clear next race mask - power only lasts 1 race
    sNextRaceSuperRarePowerMask = 0;
    
    // Clear the "just won" display flags
    sJustWonSuperRarePowerMask = 0;
    
    // Reset pre-roll state for new race
    sPreRolledSuperRarePowerMask = 0;
    sHasPreRolledThisRace = false;
}
RaceLoadHook sSuperRarePowerRaceResetHook(ResetSuperRarePowerForRace);

// Check if a player has the Super Rare Power for this race
bool HasSuperRarePower(u8 playerId) {
    if (playerId >= 12) return false;
    return (sSuperRarePowerMask & (1 << playerId)) != 0;
}

// Check if a player just won the Super Rare Power (for results screen display)
bool JustWonSuperRarePower(u8 playerId) {
    if (!IsSuperRarePowerEnabled()) return false;
    if (playerId >= 12) return false;
    return (sJustWonSuperRarePowerMask & (1 << playerId)) != 0;
}

// Clear all Super Rare Power state (called when KO session ends)
void ClearSuperRarePowerState() {
    sSuperRarePowerMask = 0;
    sJustWonSuperRarePowerMask = 0;
    sNextRaceSuperRarePowerMask = 0;
    sPreRolledSuperRarePowerMask = 0;
    sHasPreRolledThisRace = false;
}

// Get the pre-rolled mask for network sync
u16 GetPreRolledSuperRarePowerMask() {
    return sPreRolledSuperRarePowerMask;
}

// Set the pre-rolled mask from network (non-host clients)
void SetPreRolledSuperRarePowerMask(u16 mask) {
    sPreRolledSuperRarePowerMask = mask;
}

// Get the current "just won" mask for network sync
u16 GetJustWonSuperRarePowerMask() {
    return sJustWonSuperRarePowerMask;
}

// Set the "just won" mask from network (for non-host clients)
void SetJustWonSuperRarePowerMask(u16 mask) {
    sJustWonSuperRarePowerMask = mask;
}

// Get the next race mask for network sync
u16 GetNextRaceSuperRarePowerMask() {
    return sNextRaceSuperRarePowerMask;
}

// Set the next race mask from network (for non-host clients)
void SetNextRaceSuperRarePowerMask(u16 mask) {
    sNextRaceSuperRarePowerMask = mask;
}

} // namespace KO
} // namespace Pulsar
