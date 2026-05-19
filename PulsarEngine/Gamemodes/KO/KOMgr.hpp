#ifndef _PUL_KOMGR_
#define _PUL_KOMGR_

#include <kamek.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/UI/Page/Leaderboard/GPVSLeaderboardTotal.hpp>
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <MarioKartWii/Race/RaceData.hpp>
#include <PulsarSystem.hpp>

namespace Pulsar {
namespace KO {

// Player status in KO mode
enum Status {
    NORMAL,
    SUPERPOWER,
    TIE,
    KOD,
    DISCONNECTED
};

class Mgr {
public:
    // Constants
    static const u16 spectatorVote = 0x45;
    static const u32 arbitraryAlmostDied = 60; // 60 frames in danger in the last 5s = almost out

    // Structures for tracking player positions and stats
    struct PlayerPosition {
        u8 position;
        u8 playerId;
    };

    struct Stats {
        Stats() : percentageSum(0.0f) {}

        struct Final {
            Final() : timeInDanger(0), almostKOdCounter(0), finalPercentageSum(0) {}
            u16 timeInDanger;
            u8 almostKOdCounter;
            u8 finalPercentageSum; // Divided by race count at GP end
        };

        float percentageSum;
        bool isInDangerFrames[300];  // Updated each frame in race
        u32 boolCountArray;
        Final final;
    };

    // Static methods
    static void Create(Page* froom, u32 director, float length);
    static void Update();  // RaceFrameHook
    static void ProcessKOs(Pages::GPVSLeaderboardUpdate::Player* playerArr, 
                          size_t nitems, size_t size, 
                          int (*compar)(const void*, const void*));
    
    static int SortPlayersByPosition(PlayerPosition* a, PlayerPosition* b) {
        return a->position - b->position;
    }

    // Constructor/Destructor
    Mgr();
    ~Mgr();

    // Race management
    inline void ResetRace() {
        for(int i = 0; i < 2; ++i) {
            Stats& stats = this->stats[i];
            memset(&stats.isInDangerFrames[0], 0, sizeof(u8) * 300);
            stats.boolCountArray = 0;
            this->posTrackerAnmFrames[i] = 0;
        }
        for(int i = 0; i < 12; ++i) {
            if(this->status[i][0] == TIE) this->status[i][0] = NORMAL;
            if(this->status[i][1] == TIE) this->status[i][1] = NORMAL;
        }
    }
    void AddRaceStats();
    void CalcWouldBeKnockedOut();  // Checks if player would be KO'd if race ended now

    // Player status management
    Status GetAidStatus(u8 aid, u8 hudslotId) const {
        return static_cast<Status>(this->status[aid][hudslotId]);
    }

    Status GetPlayerStatus(u8 playerId) const {
        u32 aidSlot = this->GetAidAndSlotFromPlayerId(playerId);
        return this->GetAidStatus(aidSlot & 0xFFFF, aidSlot >> 16);
    }

    bool IsKOdAid(u8 aid, u8 hudslotId) const {
        return GetAidStatus(aid, hudslotId) == KOD;
    }

    bool IsDisconnectedAid(u8 aid, u8 hudslotId) const {
        return GetAidStatus(aid, hudslotId) == DISCONNECTED;
    }

    bool IsKOdPlayerId(u8 playerId) const {
        return GetPlayerStatus(playerId) == KOD;
    }

    bool IsDisconnectedPlayerId(u8 playerId) const {
        return GetPlayerStatus(playerId) == DISCONNECTED;
    }

    bool IsSuperpowerUnlocked(u8 playerId) const {
        return superpowerUnlocked[playerId];
    }

    bool HasSuperpowerStage(u8 playerId) const {
        return superpowerStage[playerId] >= 1;
    }

    bool HasSuperpowerStage2(u8 playerId) const {
        return superpowerStage[playerId] >= 2;
    }

    bool HasSuperpowerStage3(u8 playerId) const {
        return superpowerStage[playerId] >= 3;
    }

    static bool AreSuperpowersEnabled() {
        const System* system = System::sInstance;
        return system != nullptr && system->IsContext(PULSAR_MODE_KO) && system->IsContext(PULSAR_SUPERPOWERS);
    }

    // Update superpower stages locally based on current scores
    // Only called during race to track progress - stages can only increase during a race
    void UpdateSuperpowerStagesDuringRace() {
        if (!AreSuperpowersEnabled()) return;
        
        // Don't update stages if we detected a fresh KO (all previous scores were 0)
        // This prevents stale score data from granting superpowers on the first race
        if (this->isFreshKOSession) return;
        
        // Use racesScenario during actual race for live score tracking
        const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
        const u8 playerCount = scenario.playerCount;
        
        for (u8 playerId = 0; playerId < playerCount && playerId < 12; ++playerId) {
            const u16 curScore = scenario.players[playerId].score;
            u8 newStage = curScore / 20;  // 20, 40, 60 => stages 1-3
            if (newStage > 3) newStage = 3;
            // Only allow stages to increase during a race, never decrease
            if (newStage > this->superpowerStage[playerId]) {
                this->superpowerStage[playerId] = newStage;
                // Mark that this player unlocked a new superpower stage this race
                this->superpowerUnlocked[playerId] = true;
            }
        }
    }

    // Reset and sync stages from previousScore at race start
    // This ensures players only have powers they've actually earned this KO session
    void ResetSuperpowerStagesForRace() {
        // Clear unlock flags at race start - they'll be set during the race if stages increase
        for (u8 i = 0; i < 12; ++i) {
            this->superpowerUnlocked[i] = false;
        }
        
        // Check if we're forcing no superpowers (finals in multi-race KO)
        if (this->forceNoSuperpowersNextRace) {
            for (u8 i = 0; i < 12; ++i) {
                this->superpowerStage[i] = 0;
            }
            this->forceNoSuperpowersNextRace = false;  // Clear the flag after using it
            this->isFreshKOSession = true;  // Also block mid-race updates
            return;
        }
        
        if (!AreSuperpowersEnabled()) {
            // Clear all stages if superpowers are disabled
            for (u8 i = 0; i < 12; ++i) {
                this->superpowerStage[i] = 0;
            }
            this->isFreshKOSession = false;
            return;
        }
        
        // Use menusScenario for score data
        const RacedataScenario& scenario = Racedata::sInstance->menusScenario;
        const u8 playerCount = scenario.playerCount;
        
        // Check if all players have 0 previousScore - this means it's a fresh KO start
        bool allScoresZero = true;
        for (u8 playerId = 0; playerId < playerCount && playerId < 12; ++playerId) {
            if (scenario.players[playerId].previousScore != 0) {
                allScoresZero = false;
                break;
            }
        }
        
        // If all scores are 0, force reset all superpowers - fresh KO session
        if (allScoresZero) {
            for (u8 i = 0; i < 12; ++i) {
                this->superpowerStage[i] = 0;
            }
            this->isFreshKOSession = true;  // Block UpdateSuperpowerStagesDuringRace for this race
            return;
        }
        
        // Not a fresh KO, allow normal stage updates
        this->isFreshKOSession = false;
        
        // Otherwise, set stages based on previousScore from prior races
        for (u8 playerId = 0; playerId < 12; ++playerId) {
            if (playerId < playerCount) {
                // previousScore contains accumulated points from prior races in multi-race KO
                const u16 prevScore = scenario.players[playerId].previousScore;
                u8 stageFromPrev = prevScore / 20;
                if (stageFromPrev > 3) stageFromPrev = 3;
                
                // SET stage to exactly what previousScore indicates (not just increase)
                // This ensures stages are reset when scores are reset after KO rounds
                this->superpowerStage[playerId] = stageFromPrev;
            } else {
                // Clear stage for non-existent players
                this->superpowerStage[playerId] = 0;
            }
        }
    }

    // Legacy function - now just calls ResetSuperpowerStagesForRace
    void SyncSuperpowerStagesFromPreviousScores() {
        ResetSuperpowerStagesForRace();
    }

    void SetKOd(u8 playerId) { 
        this->SetStatus(playerId, KOD); 
    }

    void SetDisconnected(u8 playerId) { 
        this->SetStatus(playerId, DISCONNECTED); 
    }

    void SetTie(u8 playerId, u8 playerId2) {
        this->SetStatus(playerId, TIE);
        this->SetStatus(playerId2, TIE);
    }

    bool GetWouldBeKnockedOut(u8 playerId) const { 
        return this->wouldBeOut[playerId]; 
    }

    // Controller and UI management
    bool GetIsSwapped() const { return this->hasSwapped; }
    void SwapControllersAndUI();
    void PatchAids(RKNet::ControllerSub& sub) const;
    PageId KickPlayersOut(PageId defaultId);

    // Utility functions
    SectionId GetSectionAfterKO(SectionId defaultId) const;
    u32 GetAidAndSlotFromPlayerId(u8 playerId) const;
    u8 GetBaseLocalPlayerCount() const { return this->baseLocPlayerCount; }

private:
    void SetStatus(u8 playerId, Status status) {
        u32 aidSlot = this->GetAidAndSlotFromPlayerId(playerId);
        this->status[aidSlot & 0xFFFF][aidSlot >> 16] = status;
    }

    // Player status tracking
    u8 status[12][2];  // Indexed by [aid][hudslot]
    bool wouldBeOut[12];

    // Player management
    u8 baseLocPlayerCount;  // Player count when GP started
    bool hasSwapped;  // Controller swap status

    // Bonus tracking
    u8 superpowerStage[12];  // Highest 20-point milestone reached (0-3)
    bool superpowerUnlocked[12];  // Trigger flag for current results screen
    bool isFreshKOSession;  // True on first race of KO - blocks stale score data from granting powers
    bool forceNoSuperpowersNextRace;  // True when going to finals in multi-race KO - prevents superpowers

public:
    // Game settings
    bool isTiebreakerRace;
    u8 racesPerKO;
    u8 koPerRace;
    bool alwaysFinal;

    // Game state
    u8 winnerPlayerId;
    bool isSpectating;
    Stats stats[2];
    u8 posTrackerAnmFrames[2];
};

} // namespace KO
} // namespace Pulsar

#endif