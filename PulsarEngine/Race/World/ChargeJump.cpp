#include <kamek.hpp>
#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Item/Obj/ObjProperties.hpp>
#include <MarioKartWii/Item/Obj/Gesso.hpp>
#include <MarioKartWii/Driver/DriverManager.hpp>
#include <MarioKartWii/Input/InputManager.hpp>
#include <MarioKartWii/CourseMgr.hpp>
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <MarioKartWii/Effect/EffectMgr.hpp>
#include <core/rvl/PAD.hpp>
#include <core/rvl/WPAD.hpp>
#include <PulsarSystem.hpp>

namespace Pulsar {
namespace Race {

static const u8 CHARGE_JUMP_MIN_TIME = 5;
static const u8 CHARGE_JUMP_MAX_TIME = 60;
static const float CHARGE_JUMP_MIN_SPEED = 15.0f;
static const float CHARGE_JUMP_SINK_SCALE = 0.85f;

struct ChargeJumpState {
    u8 chargeTimer[12];
    bool isCharging[12];
    bool wasHopping[12];           // Track if player was hopping last frame
    u8 hopPendingFrames[12];       // Frames since hop started, waiting to see if drift initiates
    u8 rfhFrameCounter[12];
    Vec3 originalScale[12];
    bool hasOriginalScale[12];
};

static ChargeJumpState chargeJumpState = {};

bool g_chargeJumpUseFeather = false;

void ApplyChargeJumpFeatherEffect(Item::Player& itemPlayer) {
    const Kart::Pointers* pointers = itemPlayer.pointers;
    if (pointers == nullptr) return;
    
    Kart::Status* status = pointers->kartStatus;
    if (status == nullptr) return;
    
    Kart::Movement* kartMovement = pointers->kartMovement;
    if (kartMovement == nullptr) return;
    
    kartMovement->specialFloor |= 0x4;

    u32 type = 0x4;
    if ((status->bitfield1 & 0x4000) != 0) type = 0x2;
    status->jumpPadType = type;
    status->trickableTimer = 0x4;
}

void UseChargeJumpFeather(Item::Player& itemPlayer) {
    g_chargeJumpUseFeather = true;
    ApplyChargeJumpFeatherEffect(itemPlayer);
    g_chargeJumpUseFeather = false;
}

bool IsChargeJumpFeatherActive(u8 playerId) {
    return g_chargeJumpUseFeather;
}

void UpdateChargeJump() {
    if (!System::sInstance->IsContext(PULSAR_MAYHEM_WORLD)) {
        return;
    }

    Raceinfo* raceInfo = Raceinfo::sInstance;
    if (raceInfo == nullptr || raceInfo->stage >= RACESTAGE_IS_FINISHING) {
        return;
    }

    SectionMgr* sectionMgr = SectionMgr::sInstance;
    if (sectionMgr == nullptr) {
        return;
    }

    Item::Manager* itemManager = Item::Manager::sInstance;
    if (itemManager == nullptr) {
        return;
    }

    Racedata* racedata = Racedata::sInstance;
    if (racedata == nullptr) {
        return;
    }

    const SectionPad& pad = sectionMgr->pad;
    
    for (u8 hudSlotId = 0; hudSlotId < 4; ++hudSlotId) {
        Input::ControllerHolder* controllerHolder = pad.GetControllerHolder(hudSlotId);
        if (controllerHolder == nullptr || controllerHolder->curController == nullptr) {
            chargeJumpState.isCharging[hudSlotId] = false;
            chargeJumpState.chargeTimer[hudSlotId] = 0;
            chargeJumpState.wasHopping[hudSlotId] = false;
            chargeJumpState.rfhFrameCounter[hudSlotId] = 0;
            chargeJumpState.hopPendingFrames[hudSlotId] = 0;
            continue;
        }

        // Get actual playerId from hudSlotId
        const u32 playerId = racedata->GetPlayerIdOfLocalPlayer(hudSlotId);
        if (playerId >= itemManager->playerCount) {
            chargeJumpState.isCharging[hudSlotId] = false;
            chargeJumpState.chargeTimer[hudSlotId] = 0;
            chargeJumpState.wasHopping[hudSlotId] = false;
            chargeJumpState.rfhFrameCounter[hudSlotId] = 0;
            chargeJumpState.hopPendingFrames[hudSlotId] = 0;
            continue;
        }
        
        Item::Player& itemPlayer = itemManager->players[playerId];
        const Kart::Pointers* pointers = itemPlayer.pointers;
        if (pointers == nullptr) {
            chargeJumpState.isCharging[hudSlotId] = false;
            chargeJumpState.chargeTimer[hudSlotId] = 0;
            chargeJumpState.wasHopping[hudSlotId] = false;
            chargeJumpState.rfhFrameCounter[hudSlotId] = 0;
            chargeJumpState.hopPendingFrames[hudSlotId] = 0;
            continue;
        }

        Kart::Movement* kartMovement = pointers->kartMovement;
        if (kartMovement == nullptr) {
            chargeJumpState.isCharging[hudSlotId] = false;
            chargeJumpState.chargeTimer[hudSlotId] = 0;
            chargeJumpState.wasHopping[hudSlotId] = false;
            chargeJumpState.rfhFrameCounter[hudSlotId] = 0;
            chargeJumpState.hopPendingFrames[hudSlotId] = 0;
            continue;
        }

        Kart::Status* status = pointers->kartStatus;
        if (status == nullptr) {
            chargeJumpState.isCharging[hudSlotId] = false;
            chargeJumpState.chargeTimer[hudSlotId] = 0;
            chargeJumpState.wasHopping[hudSlotId] = false;
            chargeJumpState.rfhFrameCounter[hudSlotId] = 0;
            chargeJumpState.hopPendingFrames[hudSlotId] = 0;
            continue;
        }

        float currentSpeed = kartMovement->engineSpeed;

        // Get current input state
        u16 buttonActions = controllerHolder->inputStates[0].buttonActions;
        float stickX = controllerHolder->inputStates[0].stick.x;
        
        // Check if hop button is being held (0x8 = hop + drift)
        bool isHopButtonHeld = (buttonActions & 0x8) != 0;
        bool wasHopping = chargeJumpState.wasHopping[hudSlotId];
        
        // Detect hop start (transition from not hopping to hopping)
        bool hopJustStarted = isHopButtonHeld && !wasHopping;
        
        // Check if driving straight (stick near center)
        const float STRAIGHT_THRESHOLD = 0.3f;
        bool isDrivingStraight = (stickX > -STRAIGHT_THRESHOLD && stickX < STRAIGHT_THRESHOLD);
        
        // Number of frames to wait after hop to check if drift initiated
        const u8 HOP_CHECK_DELAY = 3;
        
        // Handle pending hop detection (waiting to see if drift initiates)
        if (hopJustStarted && !chargeJumpState.isCharging[hudSlotId] && currentSpeed >= CHARGE_JUMP_MIN_SPEED) {
            // Start the pending hop check
            chargeJumpState.hopPendingFrames[hudSlotId] = 1;
        }
        
        // If we're waiting to see if drift initiates
        if (chargeJumpState.hopPendingFrames[hudSlotId] > 0 && !chargeJumpState.isCharging[hudSlotId]) {
            // Player entered drift state - cancel pending check
            if (kartMovement->driftState != 0) {
                chargeJumpState.hopPendingFrames[hudSlotId] = 0;
            }
            // Player released hop button before check completed - cancel
            else if (!isHopButtonHeld) {
                chargeJumpState.hopPendingFrames[hudSlotId] = 0;
            }
            // Still waiting, increment counter
            else {
                chargeJumpState.hopPendingFrames[hudSlotId]++;
                
                // After delay frames, if still no drift AND driving straight, start charging!
                if (chargeJumpState.hopPendingFrames[hudSlotId] >= HOP_CHECK_DELAY) {
                    if (isDrivingStraight && kartMovement->driftState == 0) {
                        // Player hopped, didn't enter drift, and is driving straight - START CHARGE!
                        chargeJumpState.isCharging[hudSlotId] = true;
                        chargeJumpState.chargeTimer[hudSlotId] = 0;
                        chargeJumpState.rfhFrameCounter[hudSlotId] = 0;
                        
                        chargeJumpState.originalScale[hudSlotId] = kartMovement->scale;
                        chargeJumpState.hasOriginalScale[hudSlotId] = true;
                    }
                    // Reset pending counter regardless
                    chargeJumpState.hopPendingFrames[hudSlotId] = 0;
                }
            }
        }

        // Continue charging while hop button is held
        if (isHopButtonHeld && chargeJumpState.isCharging[hudSlotId]) {
            chargeJumpState.rfhFrameCounter[hudSlotId]++;
            
            if (chargeJumpState.chargeTimer[hudSlotId] < CHARGE_JUMP_MAX_TIME) {
                chargeJumpState.chargeTimer[hudSlotId]++;
            }
            
            // Suppress the normal hop while charging
            if (status->airtime == 0) {
                kartMovement->hopFrame = 0;
                kartMovement->hopVelY = 0.0f;
                kartMovement->hopPosY = 0.0f;
            }
            
            // Apply charge effects after minimum time
            if (chargeJumpState.chargeTimer[hudSlotId] >= CHARGE_JUMP_MIN_TIME) {
                // Sink the kart during charge
                if (chargeJumpState.hasOriginalScale[hudSlotId]) {
                    kartMovement->scale.y = chargeJumpState.originalScale[hudSlotId].y * CHARGE_JUMP_SINK_SCALE;
                }
                
                // Build up MT charge for visual effect
                if (kartMovement->driftState == 0) {
                    kartMovement->driftState = 1;
                    kartMovement->mtCharge = 0;
                    kartMovement->smtCharge = 0;
                }
                
                const s32 mtMax = 270;
                if (kartMovement->mtCharge < mtMax) {
                    kartMovement->mtCharge += 3;
                    if (kartMovement->mtCharge >= mtMax) {
                        kartMovement->mtCharge = mtMax;
                        kartMovement->driftState = 2;
                    }
                }
                
                // Keep kart straight during charge
                kartMovement->outsideDriftAngle = 0.0f;
                kartMovement->conservedTurn = 0.0f;
                kartMovement->effectiveTurn = 0.0f;
                kartMovement->hopStickX = 0;
            }
            
            // Cancel if speed drops too low
            if (currentSpeed < CHARGE_JUMP_MIN_SPEED) {
                if (chargeJumpState.hasOriginalScale[hudSlotId]) {
                    kartMovement->scale = chargeJumpState.originalScale[hudSlotId];
                }
                
                kartMovement->driftState = 0;
                kartMovement->mtCharge = 0;
                
                chargeJumpState.isCharging[hudSlotId] = false;
                chargeJumpState.chargeTimer[hudSlotId] = 0;
                chargeJumpState.rfhFrameCounter[hudSlotId] = 0;
                chargeJumpState.hasOriginalScale[hudSlotId] = false;
            }
        }

        // Release hop button while charging - execute the jump
        if (!isHopButtonHeld && wasHopping && chargeJumpState.isCharging[hudSlotId]) {
            // Restore original scale
            if (chargeJumpState.hasOriginalScale[hudSlotId]) {
                kartMovement->scale = chargeJumpState.originalScale[hudSlotId];
            }
            
            const s32 mtMax = 270;
            bool isMTCharged = kartMovement->mtCharge >= mtMax;
            
            // Execute jump if fully charged
            if (chargeJumpState.chargeTimer[hudSlotId] >= CHARGE_JUMP_MIN_TIME && 
                currentSpeed >= CHARGE_JUMP_MIN_SPEED && 
                isMTCharged) {
                UseChargeJumpFeather(itemPlayer);
            }
            
            // Reset drift state
            kartMovement->driftState = 0;
            kartMovement->mtCharge = 0;
            
            // Reset charge state
            chargeJumpState.isCharging[hudSlotId] = false;
            chargeJumpState.chargeTimer[hudSlotId] = 0;
            chargeJumpState.rfhFrameCounter[hudSlotId] = 0;
            chargeJumpState.hasOriginalScale[hudSlotId] = false;
        }

        // Track hop state for next frame
        chargeJumpState.wasHopping[hudSlotId] = isHopButtonHeld;
    }
}

static RaceFrameHook ChargeJumpUpdate(UpdateChargeJump);

void DrawChargeJumpDebug() {
    if (!System::sInstance->IsContext(PULSAR_MAYHEM_WORLD)) {
        return;
    }

    for (u8 hudSlotId = 0; hudSlotId < 4; ++hudSlotId) {
        if (chargeJumpState.isCharging[hudSlotId]) {
            u8 chargeLevel = chargeJumpState.chargeTimer[hudSlotId];
        }
    }
}

}   // namespace Race
}   // namespace Pulsar