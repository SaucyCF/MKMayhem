#include <kamek.hpp>
#include <MarioKartWii/Item/ItemPlayer.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <PulsarSystem.hpp>
#include <Gamemodes/KO/KOMgr.hpp>
#include <Gamemodes/KO/KOUnderdog.hpp>

namespace Pulsar {
namespace KO {

// Track which players have the underdog bonus this race (bitmask)
static u16 sUnderdogPlayerMask = 0;  // 0 = no one has it

// Track which players will be the underdog for the NEXT race (calculated before score reset)
static u16 sNextRaceUnderdogPlayerMask = 0;

// Track whether we've already applied the next race underdog this race load
static bool sNextRaceUnderdogConsumed = false;

// Check if underdog bonus feature is enabled (uses PULSAR_SUPERPOWERS context)
static inline bool IsUnderdogBonusEnabled() {
    const System* system = System::sInstance;
    return system != nullptr && system->IsContext(PULSAR_MODE_KO) && system->IsContext(PULSAR_SUPERPOWERS);
}

// Check if this is the first race of the KO (no one has earned points yet)
static inline bool IsFirstRaceOfKO() {
    const SectionMgr* sectionMgr = SectionMgr::sInstance;
    if (sectionMgr == nullptr) return true;
    // currentRaceNumber is -1 before first race, 0 after first race, etc.
    return sectionMgr->sectionParams->onlineParams.currentRaceNumber <= 0;
}

// Find the player(s) with the lowest GP points who are still in the game
// Uses previousScore for race start, or current score for results screen prediction
// Returns a bitmask of underdogs (can be multiple if tied)
// Returns 0 if no valid underdog (final race, no players, first race, etc.)
// playersAboutToBeKOd: bitmask of players who are about to be KO'd (used for next race prediction)
static u16 FindUnderdogPlayerMaskInternal(bool useCurrentScore, u16 playersAboutToBeKOd = 0) {
    const System* system = System::sInstance;
    if (system == nullptr || system->koMgr == nullptr) return 0;
    
    // No underdog on first race - everyone starts at 0 points
    if (!useCurrentScore && IsFirstRaceOfKO()) return 0;
    
    const Mgr* koMgr = system->koMgr;
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    if (controller == nullptr) return 0;
    
    // Use menusScenario for score data
    const RacedataScenario& scenario = Racedata::sInstance->menusScenario;
    const u8 playerCount = scenario.playerCount;
    if (playerCount < 3) return 0;  // Need at least 3 players for underdog bonus
    
    // Count how many players are still in (not KO'd, disconnected, or about to be KO'd)
    u8 stillInCount = 0;
    for (u8 playerId = 0; playerId < playerCount && playerId < 12; ++playerId) {
        bool isAboutToBeKOd = (playersAboutToBeKOd & (1 << playerId)) != 0;
        if (!koMgr->IsKOdPlayerId(playerId) && !koMgr->IsDisconnectedPlayerId(playerId) && !isAboutToBeKOd) {
            ++stillInCount;
        }
    }
    
    // If only 2 players remain (final race), no underdog bonus
    if (stillInCount <= 2) return 0;
    
    // First pass: find the lowest score among valid players
    u16 lowestScore = 0xFFFF;
    
    for (u8 playerId = 0; playerId < playerCount && playerId < 12; ++playerId) {
        // Skip eliminated, disconnected, or about-to-be-KO'd players
        bool isAboutToBeKOd = (playersAboutToBeKOd & (1 << playerId)) != 0;
        if (koMgr->IsKOdPlayerId(playerId) || koMgr->IsDisconnectedPlayerId(playerId) || isAboutToBeKOd) {
            continue;
        }
        
        // Use current score (for results screen) or previousScore (for race start)
        const u16 scoreToUse = useCurrentScore ? 
            scenario.players[playerId].score : 
            scenario.players[playerId].previousScore;
        
        if (scoreToUse < lowestScore) {
            lowestScore = scoreToUse;
        }
    }
    
    // Second pass: find all players tied at the lowest score
    u16 underdogMask = 0;
    
    for (u8 playerId = 0; playerId < playerCount && playerId < 12; ++playerId) {
        // Skip eliminated, disconnected, or about-to-be-KO'd players
        bool isAboutToBeKOd = (playersAboutToBeKOd & (1 << playerId)) != 0;
        if (koMgr->IsKOdPlayerId(playerId) || koMgr->IsDisconnectedPlayerId(playerId) || isAboutToBeKOd) {
            continue;
        }
        
        // Use current score (for results screen) or previousScore (for race start)
        const u16 scoreToUse = useCurrentScore ? 
            scenario.players[playerId].score : 
            scenario.players[playerId].previousScore;
        
        if (scoreToUse == lowestScore) {
            underdogMask |= (1 << playerId);
        }
    }
    
    return underdogMask;
}

// Reset underdog bonus at the start of each race
static void ResetUnderdogBonusForRace() {
    if (!IsUnderdogBonusEnabled()) {
        sUnderdogPlayerMask = 0;
        sNextRaceUnderdogConsumed = false;
        return;
    }
    
    // Use the pre-calculated next race underdog mask if available and not already consumed
    // This is critical for multi-race KO where scores get reset
    // Note: RaceLoadHook may run multiple times, so we use a flag to avoid re-consuming
    if (sNextRaceUnderdogPlayerMask != 0 && !sNextRaceUnderdogConsumed) {
        sUnderdogPlayerMask = sNextRaceUnderdogPlayerMask;
        sNextRaceUnderdogConsumed = true;  // Mark as consumed but don't clear yet (results screen still needs it)
        return;
    }
    
    // If already consumed this race load, just return (don't recalculate)
    if (sNextRaceUnderdogConsumed) {
        return;
    }
    
    // Fallback: calculate based on previousScore (for first race or single-race KO)
    sUnderdogPlayerMask = FindUnderdogPlayerMaskInternal(false);
}
RaceLoadHook sUnderdogRaceResetHook(ResetUnderdogBonusForRace);

// Check if a player has the underdog bonus
bool HasUnderdogBonus(u8 playerId) {
    if (playerId >= 12) return false;
    return (sUnderdogPlayerMask & (1 << playerId)) != 0;
}

// Get the current underdog player mask (0 if none)
u16 GetUnderdogPlayerMask() {
    return sUnderdogPlayerMask;
}

// Check if a player will be the underdog for the NEXT race
// Uses the pre-calculated sNextRaceUnderdogPlayerMask which was set before score reset
// This is used for results screen display
bool WillBeUnderdogNextRace(u8 playerId) {
    if (!IsUnderdogBonusEnabled()) return false;
    if (playerId >= 12) return false;
    
    return (sNextRaceUnderdogPlayerMask & (1 << playerId)) != 0;
}

// Calculate and store who will be the underdog for the next race
// This must be called BEFORE scores are reset in ProcessKOs
// playersAboutToBeKOd: bitmask of players who are about to be KO'd
void CalculateNextRaceUnderdog(u16 playersAboutToBeKOd) {
    if (!IsUnderdogBonusEnabled()) {
        sNextRaceUnderdogPlayerMask = 0;
        sNextRaceUnderdogConsumed = false;
        return;
    }
    
    // Reset consumed flag since we're calculating a new underdog
    sNextRaceUnderdogConsumed = false;
    
    // Use current score and exclude players about to be KO'd
    sNextRaceUnderdogPlayerMask = FindUnderdogPlayerMaskInternal(true, playersAboutToBeKOd);
}

// Clear all underdog state (called when KO session ends or room is closed)
void ClearUnderdogState() {
    sUnderdogPlayerMask = 0;
    sNextRaceUnderdogPlayerMask = 0;
    sNextRaceUnderdogConsumed = false;
}

} // namespace KO
} // namespace Pulsar
