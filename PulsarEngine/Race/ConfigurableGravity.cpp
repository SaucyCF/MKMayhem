#include <kamek.hpp>
#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/Kart/KartPhysics.hpp>
#include <MarioKartWii/Kart/KartStatus.hpp>
#include <MarioKartWii/Effect/EffectMgr.hpp> 
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <MarioKartWii/KMP/KMPManager.hpp>
#include <Race/200ccParams.hpp>
#include <PulsarSystem.hpp>

namespace Pulsar {
namespace Race {


static const u8 AREA_TYPE_GRAVITY = 0x1A; // AREA type for configurable gravity

// Forward declaration
static void CheckAndFlipUpsideDownKart(u8 playerId);

float GetAreaGravityMultiplier(u8 playerId) {
    float gravityMultiplier = 1.0f;
    
    Kart::Manager* kartMgr = Kart::Manager::sInstance;
    KMP::Manager* kmpMgr = KMP::Manager::sInstance;
    
    if (kartMgr == nullptr || kmpMgr == nullptr || kmpMgr->areaSection == nullptr) {
        return gravityMultiplier;
    }
    
    if (playerId >= kartMgr->playerCount) {
        return gravityMultiplier;
    }
    
    Kart::Player* player = kartMgr->players[playerId];
    if (player == nullptr) {
        return gravityMultiplier;
    }
    
    const Vec3& position = player->GetPhysics().position;
    
    s16 foundIdx = kmpMgr->FindAREA(position, -1, AREA_TYPE_GRAVITY);
    
    if (foundIdx >= 0) {
        AREA* area = kmpMgr->areaSection->holdersArray[foundIdx]->raw;
        float* settingAsFloat = reinterpret_cast<float*>(&area->setting1);
        gravityMultiplier = *settingAsFloat;
    }
    
    return gravityMultiplier;
}

static void ApplyAreaGravityMultiplier(Kart::Physics& physics, float dt, float maxSpeed, int air) {
    float gravityMultiplier = 1.0f;
    
    KMP::Manager* kmpMgr = KMP::Manager::sInstance;
    bool inGravityArea = false;
    
    if (kmpMgr != nullptr && kmpMgr->areaSection != nullptr) {
        const Vec3& position = physics.position;
        
        s16 foundIdx = kmpMgr->FindAREA(position, -1, AREA_TYPE_GRAVITY);
        
        if (foundIdx >= 0) {
            AREA* area = kmpMgr->areaSection->holdersArray[foundIdx]->raw;
            float* settingAsFloat = reinterpret_cast<float*>(&area->setting1);
            gravityMultiplier = *settingAsFloat;
            inGravityArea = true;
        }
    }
    
    physics.gravity *= gravityMultiplier;
    
    physics.Update(false, dt, maxSpeed);
    
    // After physics update, check if kart is upside down and flip if needed
    if (inGravityArea) {
        // Find which player this physics belongs to
        Kart::Manager* kartMgr = Kart::Manager::sInstance;
        if (kartMgr != nullptr) {
            for (u8 i = 0; i < kartMgr->playerCount; i++) {
                Kart::Player* player = kartMgr->players[i];
                if (player != nullptr && &player->GetPhysics() == &physics) {
                    CheckAndFlipUpsideDownKart(i);
                    break;
                }
            }
        }
    }
}
kmCall(0x8059fb5c, ApplyAreaGravityMultiplier);

// ============================================================================
// Upside-Down Flip Detection & Correction
// ============================================================================
// If player is in gravity AREA and their kart body is touching collision
// but wheels are NOT touching for a sustained period, flip the kart.

// Number of frames the kart must be body-on-ground but wheels-off before flipping
static const u8 UPSIDE_DOWN_FRAMES_REQUIRED = 30; // ~0.5 seconds at 60fps

// Cooldown after a flip to prevent rapid re-flipping
static const u8 FLIP_COOLDOWN_FRAMES = 60;

// Per-player state tracking
struct UpsideDownState {
    u8 framesUpsideDown;  // Counter for how long body collision without wheel collision
    u8 flipCooldown;      // Cooldown after flip
};
static UpsideDownState s_upsideDownState[4] = {
    {0, 0}, {0, 0}, {0, 0}, {0, 0}
};

static void CheckAndFlipUpsideDownKart(u8 playerId) {
    Kart::Manager* kartMgr = Kart::Manager::sInstance;
    KMP::Manager* kmpMgr = KMP::Manager::sInstance;
    
    if (kartMgr == nullptr || kmpMgr == nullptr || kmpMgr->areaSection == nullptr) {
        return;
    }
    
    if (playerId >= kartMgr->playerCount || playerId >= 4) {
        return;
    }
    
    UpsideDownState& state = s_upsideDownState[playerId];
    
    // Handle cooldown
    if (state.flipCooldown > 0) {
        state.flipCooldown--;
        state.framesUpsideDown = 0;
        return;
    }
    
    Kart::Player* player = kartMgr->players[playerId];
    if (player == nullptr) return;
    
    // Check if player is in a gravity AREA
    const Vec3& position = player->GetPhysics().position;
    s16 foundIdx = kmpMgr->FindAREA(position, -1, AREA_TYPE_GRAVITY);
    if (foundIdx < 0) {
        // Not in gravity area - reset counter
        state.framesUpsideDown = 0;
        return;
    }
    
    // Check KartStatus bitfields
    Kart::Status* kartStatus = player->pointers.kartStatus;
    if (kartStatus == nullptr) {
        state.framesUpsideDown = 0;
        return;
    }
    
    u32 bitfield0 = kartStatus->bitfield0;
    
    // Check conditions for upside-down:
    // - Floor collision with kart body (0x400) IS set (body touching ground)
    // - Floor collision with any wheel (0x800) is NOT set (wheels not touching)
    bool bodyCollision = (bitfield0 & 0x400) != 0;
    bool wheelCollision = (bitfield0 & 0x800) != 0;
    
    if (bodyCollision && !wheelCollision) {
        // Kart body is on ground but wheels are not - increment counter
        state.framesUpsideDown++;
        
        if (state.framesUpsideDown >= UPSIDE_DOWN_FRAMES_REQUIRED) {
            // Player has been upside down long enough - flip them!
            Kart::Physics& physics = player->GetPhysics();
            Quat& fullRot = physics.fullRot;
            
            // Get the kart's forward axis from current rotation
            // Forward is local Z axis: rotate (0, 0, 1) by current quaternion
            float qx = fullRot.x;
            float qy = fullRot.y;
            float qz = fullRot.z;
            float qw = fullRot.w;
            
            // Rotate (0, 0, 1) by quaternion to get forward vector
            float fwdX = 2.0f * (qx * qz + qw * qy);
            float fwdY = 2.0f * (qy * qz - qw * qx);
            float fwdZ = 1.0f - 2.0f * (qx * qx + qy * qy);
            
            // Normalize forward vector using fast inverse sqrt
            float fwdLenSq = fwdX * fwdX + fwdY * fwdY + fwdZ * fwdZ;
            if (fwdLenSq > 0.0001f) {
                float invLen = fwdLenSq;
                float halfLen = 0.5f * fwdLenSq;
                int i = *(int*)&invLen;
                i = 0x5f3759df - (i >> 1);
                invLen = *(float*)&i;
                invLen = invLen * (1.5f - halfLen * invLen * invLen);
                
                fwdX *= invLen;
                fwdY *= invLen;
                fwdZ *= invLen;
            }
            
            // Create a 180-degree rotation quaternion around the forward axis
            // For 180 degrees: w = cos(90°) = 0, (x,y,z) = sin(90°) * axis = axis
            Quat flipQuat;
            flipQuat.x = fwdX;
            flipQuat.y = fwdY;
            flipQuat.z = fwdZ;
            flipQuat.w = 0.0f;
            
            // Multiply current rotation by flip rotation: newRot = flipQuat * fullRot
            Quat newRot;
            newRot.w = flipQuat.w * fullRot.w - flipQuat.x * fullRot.x - flipQuat.y * fullRot.y - flipQuat.z * fullRot.z;
            newRot.x = flipQuat.w * fullRot.x + flipQuat.x * fullRot.w + flipQuat.y * fullRot.z - flipQuat.z * fullRot.y;
            newRot.y = flipQuat.w * fullRot.y - flipQuat.x * fullRot.z + flipQuat.y * fullRot.w + flipQuat.z * fullRot.x;
            newRot.z = flipQuat.w * fullRot.z + flipQuat.x * fullRot.y - flipQuat.y * fullRot.x + flipQuat.z * fullRot.w;
            
            // Normalize the result quaternion
            float quatLenSq = newRot.w * newRot.w + newRot.x * newRot.x + newRot.y * newRot.y + newRot.z * newRot.z;
            if (quatLenSq > 0.0001f) {
                float invLen = quatLenSq;
                float halfLen = 0.5f * quatLenSq;
                int i = *(int*)&invLen;
                i = 0x5f3759df - (i >> 1);
                invLen = *(float*)&i;
                invLen = invLen * (1.5f - halfLen * invLen * invLen);
                
                newRot.w *= invLen;
                newRot.x *= invLen;
                newRot.y *= invLen;
                newRot.z *= invLen;
            }
            
            // Apply the flipped rotation
            fullRot.x = newRot.x;
            fullRot.y = newRot.y;
            fullRot.z = newRot.z;
            fullRot.w = newRot.w;
            
            // Also update mainRot to match
            physics.mainRot.x = newRot.x;
            physics.mainRot.y = newRot.y;
            physics.mainRot.z = newRot.z;
            physics.mainRot.w = newRot.w;
            
            // Reset counter and set cooldown
            state.framesUpsideDown = 0;
            state.flipCooldown = FLIP_COOLDOWN_FRAMES;
        }
    } else {
        // Wheels are on ground or body not touching - reset counter
        state.framesUpsideDown = 0;
    }
}

}
}