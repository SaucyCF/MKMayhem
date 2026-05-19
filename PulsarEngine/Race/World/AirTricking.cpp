#include <kamek.hpp>
#include <runtimeWrite.hpp>
#include <PulsarSystem.hpp>
#include <MarioKartWii/Input/ControllerHolder.hpp>
#include <MarioKartWii/Kart/KartCollision.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <MarioKartWii/Kart/KartPhysics.hpp>
#include <MarioKartWii/Kart/KartPointers.hpp>
#include <MarioKartWii/Kart/KartStatus.hpp>
#include <MarioKartWii/Kart/KartValues.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <include/c_math.h>

namespace Pulsar {
namespace MKWorldTricking {

static const u8 MAX_PLAYERS = 12;
static const u32 DAMAGE_FLAG = 0x1;
static const u32 TRICK_START_FLAG = 0x20;
static const u32 HALFPIPE_FLAG = 0x400;
static const u32 TRICKABLE_FLAG = 0x40000000;
static const u32 STATUS0_TOUCHING_GROUND = 0x40000;
static const u32 STATUS0_HOP = 0x80000;
static const u32 STATUS0_RAMP_BOOST = 0x80000000;
static const u32 SURFACE_FLAG_BOOST_RAMP = 0x10;
static const u32 AIRTIME_REQUIRED_FOR_FIRST_TRICK = 30;
static const u32 TRICK_COOLDOWN_FRAMES = 30;
static const u32 TRICK_AIRTIME_VALUE = 5;
static const u8 TRICK_DIR_UP = 1;
static const u8 TRICK_DIR_DOWN = 2;
static const u8 TRICK_DIR_LEFT = 3;
static const u8 TRICK_DIR_RIGHT = 4;
static const float LATERAL_SHIFT_SPEED = 100.0f;

struct WorldTrickState {
	u32 airFrameCounter;
	u32 framesSinceLastTrick;  
	bool hasTrickedThisAir;
	bool waitingForRelease;
};

WorldTrickState sWorldTrickStates[MAX_PLAYERS] = {};

WorldTrickState* GetWorldTrickState(const Kart::Trick& trick) {
	Kart::Pointers* pointers = trick.pointers;
	if (pointers == nullptr) return nullptr;
	Kart::Values* values = pointers->values;
	if (values == nullptr) return nullptr;
	const u8 idx = values->playerIdx;
	if (idx >= MAX_PLAYERS) return nullptr;
	return &sWorldTrickStates[idx];
}

u8 GetTrickInput(const Input::ControllerHolder& holder) {
	return holder.inputStates[0].motionControlFlick;
}

void TickNextTimer(s16& timer) {
	if (timer <= 0) {
		timer = 0;
		return;
	}
	--timer;
	if (timer < 0) timer = 0;
}

void UpdateBoostRampFlag(Kart::Trick& trick, const Kart::Status& status) {
	if ((status.bitfield0 & STATUS0_TOUCHING_GROUND) == 0) return;
	Kart::Collision& collision = trick.GetCollision();
	if ((collision.surfaceProperties & SURFACE_FLAG_BOOST_RAMP) == 0) {
		trick.boostRampEnabled = false;
	}
}

bool HasTrickableSurface(const Kart::Status& status) {
	if ((status.bitfield1 & TRICKABLE_FLAG) != 0) return true;
	return static_cast<s32>(status.boostRampType) >= 0;
}

void ApplyLateralShift(Kart::Trick& trick, u8 trickDir) {
	if (trickDir != TRICK_DIR_LEFT && trickDir != TRICK_DIR_RIGHT) return;
	
	// Get the movement component to access facing direction
	Kart::Movement& movement = trick.GetMovement();
	
	// Get the physics to modify position
	Kart::Physics& physics = trick.GetPhysics();
	
	// Get the kart's forward direction (normalized direction in XZ plane)
	const Vec3& forward = movement.dir;
	
	float rightX = forward.z;
	float rightZ = -forward.x;
	
	// Normalize the right vector
	float len = static_cast<float>(sqrt(static_cast<double>(rightX * rightX + rightZ * rightZ)));
	if (len > 0.001f) {
		rightX /= len;
		rightZ /= len;
	}
	
	// Determine shift direction: left trick = negative right, right trick = positive right
	float shiftSign = (trickDir == TRICK_DIR_LEFT) ? 1.0f : -1.0f;
	
	// Calculate offset
	float offsetX = rightX * LATERAL_SHIFT_SPEED * shiftSign;
	float offsetZ = rightZ * LATERAL_SHIFT_SPEED * shiftSign;
	
	// Directly modify the kart's position
	physics.position.x += offsetX;
	physics.position.z += offsetZ;
}

static void UpdateNextTrick(Kart::Trick* trickRaw) {
	if (trickRaw == nullptr) return;
	Kart::Trick& trick = *trickRaw;
	Input::ControllerHolder& holder = trick.GetControllerHolder();
	const u8 trickDir = GetTrickInput(holder);
	const bool trickPressed = trickDir != 0;

	Kart::Pointers* pointers = trick.pointers;
	if (pointers == nullptr) return;
	Kart::Status* status = pointers->kartStatus;
	if (status == nullptr) return;

	WorldTrickState* worldState = GetWorldTrickState(trick);
	if (worldState == nullptr) return;

	const bool onGround = (status->bitfield0 & STATUS0_TOUCHING_GROUND) != 0;
	const bool damaged = (status->bitfield1 & DAMAGE_FLAG) != 0;

	// Track airtime and cooldown - reset when touching ground
	if (onGround) {
		worldState->airFrameCounter = 0;
		worldState->framesSinceLastTrick = 0;
		worldState->hasTrickedThisAir = false;
		worldState->waitingForRelease = false;
	} else {
		// In the air, increment counters
		if (worldState->airFrameCounter < 0xFFFFFFFF) {
			worldState->airFrameCounter++;
		}
		if (worldState->framesSinceLastTrick < 0xFFFFFFFF) {
			worldState->framesSinceLastTrick++;
		}
	}

	// Clear waitingForRelease when trick input is released
	if (!trickPressed && worldState->waitingForRelease) {
		worldState->waitingForRelease = false;
	}

	// Store trick input direction for standard trick handling
	if (!damaged && trickPressed) {
		trick.nextDirection = trickDir;
		trick.nextTimer = 0xE;
	}

	// World trick logic
	// First trick: requires AIRTIME_REQUIRED_FOR_FIRST_TRICK frames in air (unless hopping)
	// Subsequent tricks: requires TRICK_COOLDOWN_FRAMES since last trick
	bool canWorldTrick = false;
	if (!worldState->hasTrickedThisAir) {
		// First trick this jump - need specified frames of airtime OR be hopping
		canWorldTrick = (worldState->airFrameCounter >= AIRTIME_REQUIRED_FOR_FIRST_TRICK);
	} else {
		// Already tricked - need cooldown frames since last trick
		canWorldTrick = (worldState->framesSinceLastTrick >= TRICK_COOLDOWN_FRAMES);
	}

	bool worldTrickTriggered = false;
	if (trickPressed && !damaged && !onGround && canWorldTrick && !worldState->waitingForRelease) {
		// Force the game to think we're in the trickable window
		status->boostRampType = 1;
		if ((status->bitfield1 & HALFPIPE_FLAG) == 0) {
			status->airtime = TRICK_AIRTIME_VALUE;  // Force into 3-10 window
		}
		trick.boostRampEnabled = true;
		
		// Reset cooldown and set flags
		worldState->framesSinceLastTrick = 0;
		worldState->hasTrickedThisAir = true;
		worldState->waitingForRelease = true;  // Must release input before next trick
		worldTrickTriggered = true;
	}

	// Standard trick processing for ramps/zippers
	const s32 airtime = static_cast<s32>(status->airtime);
	if (airtime == 0 || trick.nextTimer < 1 || airtime > 10 || !HasTrickableSurface(*status) || damaged) {
		TickNextTimer(trick.nextTimer);
	}
	else {
		if (airtime > 2) {
			status->bitfield1 |= TRICK_START_FLAG; // mark trick start so anim triggers immediately
			
			// Apply lateral shift only when trick actually starts (for world tricks)
			if (worldTrickTriggered) {
				ApplyLateralShift(trick, trickDir);
			}
		}
		if ((status->bitfield0 & STATUS0_RAMP_BOOST) != 0) {
			trick.boostRampEnabled = true;
		}
	}

	UpdateBoostRampFlag(trick, *status);
}
kmRuntimeUse(0x80575b38);
static void WorldTrickingToggle() {
  kmRuntimeWrite32A(0x80575b38, 0x9421FFF0);
  if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL ||
      RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_JOINING_REGIONAL) {
      if (System::sInstance->IsContext(Pulsar::PULSAR_MAYHEM_WORLD)) {
          kmRuntimeBranchA(0x80575b38, UpdateNextTrick);
    }
  }
}
SectionLoadHook WorldTrickingPatch(WorldTrickingToggle);
} // namespace MKWorldTricking
} // namespace Pulsar