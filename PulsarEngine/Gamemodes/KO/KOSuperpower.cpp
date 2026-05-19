#include <kamek.hpp>
#include <runtimeWrite.hpp>
#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <MarioKartWii/Kart/KartPointers.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <PulsarSystem.hpp>
#include <Gamemodes/KO/KOMgr.hpp>

namespace Pulsar {
namespace KO {

static u16 sStage3ProtectionFrames[12];
static float sStage3BaseHardSpeedLimit[12];
static bool sInitializedThisRace = false;
static const u16 kStage3DurationFrames = 10 * 60; // 10 seconds
static bool sStage2Patched = false;

kmRuntimeUse(0x80797FE8);

static inline bool SuperpowersEnabled() {
	return Mgr::AreSuperpowersEnabled();
}

// Reset superpower context before each race starts
// This ensures players only have powers they've earned based on their current previousScore
static void ResetSuperpowerContextForRace() {
	System* system = System::sInstance;
	if (system == nullptr) return;
	
	Mgr* koMgr = system->koMgr;
	if (koMgr == nullptr) return;
	
	// Reset stages based on previousScore - if KO just started or scores were reset,
	// previousScore will be 0 and no one will have powers
	koMgr->ResetSuperpowerStagesForRace();
	
	// Also reset the local tracking state
	memset(sStage3ProtectionFrames, 0, sizeof(sStage3ProtectionFrames));
	memset(sStage3BaseHardSpeedLimit, 0, sizeof(sStage3BaseHardSpeedLimit));
	sInitializedThisRace = false;
	
	// Reset the stage 2 patch state
	if (sStage2Patched) {
		kmRuntimeWrite32A(0x80797FE8, 0x2C030000);
		sStage2Patched = false;
	}
}
RaceLoadHook sSuperpowerRaceResetHook(ResetSuperpowerContextForRace);

static void UpdateStage2Patch() {
	System* system = System::sInstance;
	bool shouldPatch = false;
	if (SuperpowersEnabled()) {
		Kart::Manager* kartMgr = Kart::Manager::sInstance;
		const KO::Mgr* mgr = system->koMgr;
		if (kartMgr != nullptr && mgr != nullptr) {
			const u8 playerCount = kartMgr->playerCount;
			for (u8 i = 0; i < playerCount; ++i) {
				Kart::Player* player = kartMgr->players[i];
				if (player == nullptr || !player->IsLocal()) continue;
				Kart::Values* values = player->pointers.values;
				if (values == nullptr) continue;
				const u8 slot = values->playerIdx;
				if (slot >= 12) continue;
				if (mgr->HasSuperpowerStage2(slot)) {
					shouldPatch = true;
					break;
				}
			}
		}
	}

	if (shouldPatch && !sStage2Patched) {
		kmRuntimeWrite32A(0x80797FE8, 0x2C03FFFF);
		sStage2Patched = true;
	} else if (!shouldPatch && sStage2Patched) {
		kmRuntimeWrite32A(0x80797FE8, 0x2C030000);
		sStage2Patched = false;
	}
}

static void ApplyStage3ProtectionPerFrame() {
	System* system = System::sInstance;
	if (system == nullptr || !SuperpowersEnabled()) {
		memset(sStage3ProtectionFrames, 0, sizeof(sStage3ProtectionFrames));
		memset(sStage3BaseHardSpeedLimit, 0, sizeof(sStage3BaseHardSpeedLimit));
		sInitializedThisRace = false;
		UpdateStage2Patch();
		return;
	}

	Mgr* koMgr = system->koMgr;
	Raceinfo* raceinfo = Raceinfo::sInstance;
	if (raceinfo == nullptr || raceinfo->stage != RACESTAGE_RACE) {
		memset(sStage3ProtectionFrames, 0, sizeof(sStage3ProtectionFrames));
		memset(sStage3BaseHardSpeedLimit, 0, sizeof(sStage3BaseHardSpeedLimit));
		sInitializedThisRace = false;
		UpdateStage2Patch();
		return;
	}

	// On race start, sync stages from previous scores and initialize timers
	if (!sInitializedThisRace) {
		RaceTimerMgr* timerMgr = raceinfo->timerMgr;
		const bool isRaceStart = timerMgr != nullptr && timerMgr->raceFrameCounter == 0;
		if (isRaceStart) {
			Kart::Manager* manager = Kart::Manager::sInstance;
			if (koMgr != nullptr && manager != nullptr) {
				// NOTE: Stage sync is already handled by RaceLoadHook calling ResetSuperpowerStagesForRace()
				// Don't call SyncSuperpowerStagesFromPreviousScores() here as it may overwrite the reset
				// with stale data if currentRaceNumber has changed since RaceLoadHook ran

				const u8 playerCount = manager->playerCount;
				for (u8 i = 0; i < playerCount; ++i) {
					Kart::Player* player = manager->players[i];
					if (player == nullptr) continue;
					Kart::Values* values = player->pointers.values;
					if (values == nullptr) continue;
					const u8 slot = values->playerIdx;
					if (slot >= 12) continue;
					const bool stage3 = koMgr->HasSuperpowerStage3(slot);
					sStage3ProtectionFrames[slot] = stage3 ? kStage3DurationFrames : 0;

					Kart::Movement* movement = player->pointers.kartMovement;
					if (movement != nullptr) {
						sStage3BaseHardSpeedLimit[slot] = movement->hardSpeedLimit;
					} else {
						sStage3BaseHardSpeedLimit[slot] = 0.0f;
					}
				}
			}
			sInitializedThisRace = true;
		}
	}

	// During the race, update stages based on live scores (stages can only increase)
	if (koMgr != nullptr) {
		koMgr->UpdateSuperpowerStagesDuringRace();
	}

	// Apply protection if active
	Kart::Manager* manager = Kart::Manager::sInstance;
	if (manager == nullptr) return;

	const u8 playerCount = manager->playerCount;
	for (u8 i = 0; i < playerCount; ++i) {
		Kart::Player* player = manager->players[i];
		if (player == nullptr) continue;
		Kart::Pointers& pointers = player->pointers;
		Kart::Values* values = pointers.values;
		if (values == nullptr) continue;
		const u8 slot = values->playerIdx;
		if (slot >= 12) continue;

		u16& framesLeft = sStage3ProtectionFrames[slot];
		if (framesLeft == 0) continue;

		Kart::Movement* movement = pointers.kartMovement;
		Kart::Status* status = pointers.kartStatus;
		if (movement != nullptr) {
			if (movement->starTimer < static_cast<s16>(framesLeft)) {
				movement->starTimer = static_cast<s16>(framesLeft);
			}

			const float baseLimit = sStage3BaseHardSpeedLimit[slot];
			if (baseLimit > 0.0f) {
				movement->hardSpeedLimit = baseLimit;
				if (movement->engineSpeed > baseLimit) movement->engineSpeed = baseLimit;
			}
		}
		if (status != nullptr) {
			status->bitfield1 |= 0x80000000; // mark as in star state for invincibility/unbumpable
		}

		if (framesLeft > 0) --framesLeft;
	}

	UpdateStage2Patch();
}

RaceFrameHook sStage3ProtectionHook(ApplyStage3ProtectionPerFrame);

} // namespace KO
} // namespace Pulsar
