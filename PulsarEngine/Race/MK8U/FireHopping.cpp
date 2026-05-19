#include <kamek.hpp>
#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <MarioKartWii/Kart/KartStatus.hpp>
#include <MarioKartWii/Kart/KartPointers.hpp>
#include <MarioKartWii/Kart/KartPlayer.hpp>
#include <MarioKartWii/Race/Racedata.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <PulsarSystem.hpp>

namespace Pulsar {
namespace Race {

// Configuration constants
static const s16 FIRE_HOP_WINDOW_FRAMES = 40;
static const s16 FIRE_HOP_MIN_BOOST_FRAMES = 5;
static const float FIRE_HOP_SPEED_PRESERVATION = 0.98f;

// Fire hop state for each player
struct FireHopState {
    s16 hopChainTimer;
    s16 chainCount;
    float preservedSpeed;
    bool isFireHopping;
    bool wasHopping;
    bool wasBoosting;
    bool wasGrounded;
};

static FireHopState fireHopStates[12] = {};

// Reset fire hop state for a player
static void ResetFireHopState(u8 playerId) {
    if (playerId >= 12) return;
    FireHopState& state = fireHopStates[playerId];
    state.hopChainTimer = 0;
    state.chainCount = 0;
    state.preservedSpeed = 0.0f;
    state.isFireHopping = false;
    state.wasHopping = false;
    state.wasBoosting = false;
    state.wasGrounded = false;
}

// Reset all fire hop states
static void ResetAllFireHopStates() {
    for (u8 i = 0; i < 12; ++i) {
        ResetFireHopState(i);
    }
}

// Check if a player is currently boosting (any type of boost)
static bool IsPlayerBoosting(const Kart::Status* status, const Kart::Movement* movement) {
    if (status == nullptr || movement == nullptr) return false;
    
    if ((status->bitfield0 & 0x100000) != 0) return true;  // BOOST flag
    if (movement->boost.types != 0) return true;
    if ((status->bitfield1 & 0x100000) != 0) return true;  // MT_BOOST flag
    if ((status->bitfield0 & 0x80000000) != 0) return true;  // RAMP_BOOST flag
    
    return false;
}

void UpdateFireHopping() {
    if (System::sInstance == nullptr) {
        return;
    }
    
    if (!System::sInstance->IsContext(PULSAR_MAYHEM_MK8U)) {
        return;
    }
    
    if (Racedata::sInstance == nullptr) {
        return;
    }

    Raceinfo* raceInfo = Raceinfo::sInstance;
    if (raceInfo == nullptr) {
        return;
    }
    
    if (raceInfo->stage < RACESTAGE_COUNTDOWN || raceInfo->stage >= RACESTAGE_IS_FINISHING) {
        return;
    }
    
    Kart::Manager* kartManager = Kart::Manager::sInstance;
    if (kartManager == nullptr) {
        return;
    }
    
    const u8 playerCount = Racedata::sInstance->racesScenario.playerCount;
    if (playerCount == 0 || playerCount > 12) {
        return;
    }
    
    for (u8 playerId = 0; playerId < playerCount && playerId < 12; ++playerId) {
        Kart::Player* kartPlayer = kartManager->GetKartPlayer(playerId);
        if (kartPlayer == nullptr) {
            ResetFireHopState(playerId);
            continue;
        }
        
        const Kart::Pointers& pointers = kartPlayer->pointers;
        Kart::Status* status = pointers.kartStatus;
        Kart::Movement* movement = pointers.kartMovement;
        
        if (status == nullptr || movement == nullptr) {
            ResetFireHopState(playerId);
            continue;
        }
        
        FireHopState& state = fireHopStates[playerId];
        
        // Get current state
        const bool isHopping = (status->bitfield0 & 0x80000) != 0;
        const bool isGrounded = (status->bitfield0 & 0x40000) != 0;
        const bool isBoosting = IsPlayerBoosting(status, movement);
        
        // Detect hop start (transition from not hopping to hopping while grounded)
        const bool hopStarted = isHopping && !state.wasHopping && isGrounded;
        
        // Detect landing (transition from not grounded to grounded)
        const bool justLanded = isGrounded && !state.wasGrounded;
        
        // Increment hop chain timer when we're in a fire hop chain and grounded
        if (state.isFireHopping && isGrounded && !isHopping) {
            state.hopChainTimer++;
        }
        
        // Reset timer when we hop (we're tracking time on ground between hops)
        if (hopStarted) {
            state.hopChainTimer = 0;
        }
        
        // Check if we should start or continue a fire hop chain
        if (hopStarted && isBoosting) {
            
            if (!state.isFireHopping) {
                state.isFireHopping = true;
                state.chainCount = 1;
                state.hopChainTimer = 0;
                state.preservedSpeed = movement->engineSpeed;
            } else {
                if (state.hopChainTimer <= FIRE_HOP_WINDOW_FRAMES) {
                    state.chainCount++;
                    state.hopChainTimer = 0;
                    
                    if (movement->boost.mtFrames > 0 && movement->boost.mtFrames < 200) {
                        movement->boost.mtFrames += FIRE_HOP_MIN_BOOST_FRAMES;
                    }
                    if (movement->boost.mushroomBoostPanelFrames > 0 && movement->boost.mushroomBoostPanelFrames < 200) {
                        movement->boost.mushroomBoostPanelFrames += FIRE_HOP_MIN_BOOST_FRAMES;
                    }
                    if (movement->boost.trickZipperFrames > 0 && movement->boost.trickZipperFrames < 200) {
                        movement->boost.trickZipperFrames += FIRE_HOP_MIN_BOOST_FRAMES;
                    }
                    
                    if (movement->engineSpeed < state.preservedSpeed * FIRE_HOP_SPEED_PRESERVATION) {
                        movement->engineSpeed = state.preservedSpeed * FIRE_HOP_SPEED_PRESERVATION;
                    }
                } else {
                    state.chainCount = 1;
                    state.hopChainTimer = 0;
                    state.preservedSpeed = movement->engineSpeed;
                }
            }
        } else if (state.isFireHopping) {
            
            bool shouldEndChain = false;
            
            if (!isBoosting && !state.wasBoosting) {
                shouldEndChain = true;
            }
            
            if (state.hopChainTimer > FIRE_HOP_WINDOW_FRAMES && !isHopping) {
                shouldEndChain = true;
            }
            
            if (status->airtime > (u32)(FIRE_HOP_WINDOW_FRAMES * 2)) {
                shouldEndChain = true;
            }
            
            if (shouldEndChain) {
                ResetFireHopState(playerId);
            }
        }
        
        state.wasHopping = isHopping;
        state.wasBoosting = isBoosting;
        state.wasGrounded = isGrounded;
    }
}
RaceFrameHook FireHoppingUpdate(UpdateFireHopping);

static void OnRaceLoad() {
    ResetAllFireHopStates();
}
RaceLoadHook FireHoppingRaceLoad(OnRaceLoad);

}  // namespace Race
}  // namespace Pulsar