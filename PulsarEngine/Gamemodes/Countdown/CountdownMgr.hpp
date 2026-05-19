#ifndef _PUL_COUNTDOWN_MGR_
#define _PUL_COUNTDOWN_MGR_

#include <kamek.hpp>
#include <PulsarSystem.hpp>

namespace Pulsar {
namespace Countdown {

class Mgr {
public:
    static const u8  MAX_SCORE = 10;
    static const u8  MAX_SCORE_DISPLAY = 11;
    static const u32 DEFAULT_START_SECONDS = 30;
    static const u32 FRAMES_PER_SECOND = 60;
    static const u32 DEFAULT_START_FRAMES = DEFAULT_START_SECONDS * FRAMES_PER_SECOND;
    static const u32 TIME_BONUS_FRAMES = 5 * FRAMES_PER_SECOND;
    static const u32 TIME_BONUS_SECONDS = 5;
    static const u16 NO_LAST_ITEM = 0xFFFF;
    static const u32 HIT_COOLDOWN_FRAMES = 6;
    static const u32 SAME_ITEM_COOLDOWN_FRAMES = 180;
    static const u32 SAME_VICTIM_COOLDOWN_FRAMES = 90;

    static u32 StartFramesToMs(u32 frames) { return frames * 1000u / FRAMES_PER_SECOND; }

    static u8 GetLocalPlayerId();

    Mgr();
    ~Mgr() {}

    void OnRaceLoad();
    void OnRaceFrame();
    void OnItemHit(u8 ownerPlayerId, u8 victimPlayerId, u16 itemInstanceIdx);
    void OnSoloHit(u8 ownerPlayerId, u16 itemInstanceIdx);
    void SeedReversedTimer(u32 totalSeconds);
    void FinalizeOfflinePlacementsAndEnd();
    void FinalizeOnlinePlacementsAndEnd();
    void FreezeOfflineExpiredCpu(u8 playerId);
    bool IsLocalSpectating() const { return this->localSpectating; }
    bool IsOfflineExpired(u8 playerId) const {
        if (playerId >= 12) return false;
        return this->offlineExpired[playerId];
    }

    // Network sync
    void PackForNetwork(u8* outScores, u32* outTicks) const;
    void UnpackFromNetwork(u8 senderAid, const u8* inScores, const u32* inTicks);

    // Read accessors for the HUD.
    u32 GetTicksRemaining(u8 playerId) const {
        if (playerId >= 12) return 0;
        return this->ticksRemaining[playerId];
    }
    u8 GetScore(u8 playerId) const {
        if (playerId >= 12) return 0;
        return this->score[playerId];
    }
    u32 GetStartFrames() const { return this->startFrames; }

private:
    u32 startFrames;
    u32 ticksRemaining[12];
    u8  score[12];
    u16 lastScoredItemIdx[12];
    u32 lastScoredFrame[12];   // race-frame number the aggressor last scored
    u16 lastScoredVictimItemIdx[12][12];
    u32 lastScoredVictimFrame[12][12];
    u32 overtimeFrames[12];    // frames elapsed after countdown reaches 0
    bool raceEnded[12];        // EndPlayerRace already called for this player
    bool offlineExpired[12];   // offline-only: timer hit 0 (parked, not yet EndRace)
    u8 offlineElimOrder[12];   // offline-only: expiry order oldest->newest
    u8 offlineElimCount;       // count of valid entries in offlineElimOrder
    bool localSpectating;      // true once any local timer expires (forces spectate behavior)

    // Online-only state. onlineExpired[pid] flips true the frame their timer
    // hits 0 (used to force-spectate local + gate the all-expired finalize).
    // onlineElimOrder is kept for diagnostics / tie-break needs but final
    // placements come from vanilla's playerIdInEachPosition at finalize time
    // — i.e. natural race progress just like a normal race.
    bool onlineExpired[12];
    u8   onlineElimOrder[12];
    u8   onlineElimCount;
public:
    bool IsOnlineExpired(u8 playerId) const {
        if (playerId >= 12) return false;
        return this->onlineExpired[playerId];
    }
};

}  // namespace Countdown
}  // namespace Pulsar

#endif
