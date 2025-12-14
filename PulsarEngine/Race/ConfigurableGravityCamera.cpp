#include <kamek.hpp>
#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/Kart/KartPhysics.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <MarioKartWii/Kart/KartStatus.hpp>
#include <MarioKartWii/KMP/KMPManager.hpp>
#include <MarioKartWii/3D/Camera/RaceCamera.hpp>
#include <MarioKartWii/3D/Camera/CameraBinaries.hpp>
#include <core/egg/Math/Quat.hpp>

// Extern declaration for the original camera falling function - resolved by linker for each region
// Must be outside namespace to match symbol: CameraFallingAdjustment__FiPfPvi
extern void CameraFallingAdjustment(int cameraData, float* cameraParams, void* kartProxy, int dirFlag);

namespace Pulsar {
namespace Race {

extern float GetAreaGravityMultiplier(u8 playerId);

static u8 GetPlayerIdFromKartProxy(void* kartProxy) {
    if (kartProxy == nullptr) return 0;
    
    Kart::Manager* kartMgr = Kart::Manager::sInstance;
    if (kartMgr == nullptr) return 0;
    
    for (u8 i = 0; i < kartMgr->playerCount; i++) {
        Kart::Player* player = kartMgr->players[i];
        if (player != nullptr) {
            if ((void*)player == kartProxy) {
                return i;
            }
        }
    }
    
    return 0;
}

void CameraFallingAdjustment_Hook(int cameraData, float* cameraParams, void* kartProxy, int dirFlag) {
    u8 playerId = GetPlayerIdFromKartProxy(kartProxy);
    float gravityMult = GetAreaGravityMultiplier(playerId);
    

    if (gravityMult != 1.0f) {
        float interpRate = 0.1f;
        cameraParams[6] = cameraParams[6] * (1.0f - interpRate);  
        return;  
    }
    
    // Call the original camera falling adjustment function (linker resolves address per region)
    CameraFallingAdjustment(cameraData, cameraParams, kartProxy, dirFlag);
}

kmCall(0x805a3860, CameraFallingAdjustment_Hook);

// ============================================================================
// Camera Up Vector Hook - Makes camera rotate with kart in gravity AREA boxes
// ============================================================================

// Interpolation speed (0.0 to 1.0) - higher = faster transition
static const float CAMERA_INTERP_SPEED_ENTER = 0.04f;  // ~25 frames to fully enter
static const float CAMERA_INTERP_SPEED_EXIT = 0.02f;   // ~50 frames to fully exit
static const float CAMERA_HIT_DETACH_SPEED = 0.15f;    // Fast detach when hit

// Persistent state for camera per player
struct CameraGravityState {
    float camBackDist;    // Distance behind kart (from game params)
    float camUpDist;      // Distance above kart (from game params)
    float targetOffset;   // Target offset above kart (from game params)
    float blendFactor;    // 0.0 = normal camera, 1.0 = gravity camera
    bool wasInGravityArea; // Previous frame state
    bool wasHit;          // Previous frame hit state
};
static CameraGravityState s_camState[4] = {
    {1200.0f, 350.0f, 120.0f, 0.0f, false, false},
    {1200.0f, 350.0f, 120.0f, 0.0f, false, false},
    {1200.0f, 350.0f, 120.0f, 0.0f, false, false},
    {1200.0f, 350.0f, 120.0f, 0.0f, false, false}
};

// Helper to compute sqrt
static inline float mySqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    float guess = x * 0.5f;
    for (int i = 0; i < 4; i++) {
        guess = 0.5f * (guess + x / guess);
    }
    return guess;
}

// Linear interpolation helper
static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// This function modifies camera with smooth interpolation in gravity areas
void ModifyCameraUpVector(RaceCamera* camera) {
    // Get player ID from camera
    u8 playerId = camera->playerId;
    if (playerId >= 4) return;
    
    CameraGravityState& state = s_camState[playerId];
    
    // Check if player is in a gravity area
    float gravityMult = GetAreaGravityMultiplier(playerId);
    bool inGravityArea = (gravityMult != 1.0f);
    
    // Check if player is using backwards/rearview camera (bitfield 0x20)
    bool isRearviewCamera = (camera->bitfield & 0x20) != 0;
    
    // Get kart data to check for hit/stun state
    Kart::Manager* kartMgr = Kart::Manager::sInstance;
    if (kartMgr == nullptr || playerId >= kartMgr->playerCount) return;
    
    Kart::Player* player = kartMgr->players[playerId];
    if (player == nullptr) return;
    
    // Check if player is in a hit/stun state
    // Access KartStatus through pointers (player->GetKartPointers()->kartStatus)
    Kart::Status* kartStatus = player->pointers.kartStatus;
    bool isHit = false;
    
    if (kartStatus != nullptr) {
        // bitfield1 bit 0 (0x1) = hit by an item or object
        // bitfield2 bit 7 (0x80) = shocked (Zappers/TC/Shock)
        // bitfield2 bit 16 (0x10000) = crushed (Thwomp/Mega)
        bool hitByItem = (kartStatus->bitfield1 & 0x1) != 0;
        bool isShocked = (kartStatus->bitfield2 & 0x80) != 0;
        bool isCrushed = (kartStatus->bitfield2 & 0x10000) != 0;
        isHit = hitByItem || isShocked || isCrushed;
    }
    
    // Also check KartMovement timers for ongoing stun
    Kart::Movement* movement = player->pointers.kartMovement;
    if (movement != nullptr) {
        s16 shockTimer = movement->shockTimer;
        s16 crushTimer = movement->crushTimer;
        if (shockTimer > 0 || crushTimer > 0) {
            isHit = true;
        }
    }
    
    // Determine if we should use gravity camera
    // Do NOT use gravity camera if:
    // - Player is hit/stunned (camera should not spin with kart)
    // - Player is using rearview camera (use default backwards view)
    bool shouldUseGravityCamera = inGravityArea && !isHit && !isRearviewCamera;
    
    // Update blend factor based on conditions
    if (shouldUseGravityCamera) {
        // Entering or inside gravity area with no hit/rearview - increase blend toward 1.0
        state.blendFactor += CAMERA_INTERP_SPEED_ENTER;
        if (state.blendFactor > 1.0f) state.blendFactor = 1.0f;
    } else if (isHit) {
        // Player got hit while in gravity mode - INSTANT detach (no interpolation)
        state.blendFactor = 0.0f;
    } else if (isRearviewCamera && state.blendFactor > 0.0f) {
        // Using rearview camera - use default camera behavior (fast transition)
        state.blendFactor -= CAMERA_HIT_DETACH_SPEED;
        if (state.blendFactor < 0.0f) state.blendFactor = 0.0f;
    } else {
        // Outside gravity area or conditions not met - decrease blend toward 0.0
        state.blendFactor -= CAMERA_INTERP_SPEED_EXIT;
        if (state.blendFactor < 0.0f) state.blendFactor = 0.0f;
    }
    
    // Track hit state for next frame
    state.wasHit = isHit;
    
    // If blend factor is 0, we're fully in normal mode - nothing to do
    if (state.blendFactor <= 0.0f) {
        state.wasInGravityArea = false;
        return;
    }
    
    // Access camera vectors (game has already computed these)
    EGG::Vector3f* camUp = (EGG::Vector3f*)((u8*)camera + 0x7c);       // targetDir (up vector)
    EGG::Vector3f* camPos = (EGG::Vector3f*)((u8*)camera + 0x64);      // position
    EGG::Vector3f* camTarget = (EGG::Vector3f*)((u8*)camera + 0x70);   // targetPosition (look at point)
    
    // Access the view matrix at offset 0x4 (inherited from LookAtCamera)
    float* viewMtx = (float*)((u8*)camera + 0x4);
    
    // Store the game's computed camera values (for blending)
    float gameCamPosX = camPos->x;
    float gameCamPosY = camPos->y;
    float gameCamPosZ = camPos->z;
    float gameTargetX = camTarget->x;
    float gameTargetY = camTarget->y;
    float gameTargetZ = camTarget->z;
    float gameUpX = camUp->x;
    float gameUpY = camUp->y;
    float gameUpZ = camUp->z;
    
    // Get the kart's rotation and position (reuse player pointer from above)
    Kart::Physics& physics = player->GetPhysics();
    EGG::Quatf& kartRot = (EGG::Quatf&)physics.fullRot;
    
    // Get kart position from physics
    float* physicsData = (float*)&physics;
    float kartPosX = physicsData[0x68/4];
    float kartPosY = physicsData[0x6C/4];
    float kartPosZ = physicsData[0x70/4];
    
    // Get kart's local axes
    EGG::Vector3f defaultUp(0.0f, 1.0f, 0.0f);
    EGG::Vector3f defaultFwd(0.0f, 0.0f, 1.0f);
    EGG::Vector3f kartUp, kartFwd;
    kartRot.RotateVector(defaultUp, kartUp);
    kartRot.RotateVector(defaultFwd, kartFwd);
    
    // Update camera params when first entering gravity area
    if (inGravityArea && !state.wasInGravityArea) {
        CameraParamBin::Entry* camParams = camera->camParams;
        if (camParams != nullptr) {
            state.camBackDist = camParams->cameraDist;
            state.camUpDist = camParams->yPosition;
            state.targetOffset = camParams->yTargetPos;
        }
    }
    state.wasInGravityArea = inGravityArea;
    
    // Calculate gravity-modified camera position
    float gravCamPosX = kartPosX - kartFwd.x * state.camBackDist + kartUp.x * state.camUpDist;
    float gravCamPosY = kartPosY - kartFwd.y * state.camBackDist + kartUp.y * state.camUpDist;
    float gravCamPosZ = kartPosZ - kartFwd.z * state.camBackDist + kartUp.z * state.camUpDist;
    
    // Calculate gravity-modified target position (slightly above kart center)
    float gravTargetX = kartPosX + kartUp.x * state.targetOffset;
    float gravTargetY = kartPosY + kartUp.y * state.targetOffset;
    float gravTargetZ = kartPosZ + kartUp.z * state.targetOffset;
    
    // Gravity-modified up vector is kart's up
    float gravUpX = kartUp.x;
    float gravUpY = kartUp.y;
    float gravUpZ = kartUp.z;
    
    // Interpolate between game camera and gravity camera based on blend factor
    float t = state.blendFactor;
    
    float finalCamPosX = lerp(gameCamPosX, gravCamPosX, t);
    float finalCamPosY = lerp(gameCamPosY, gravCamPosY, t);
    float finalCamPosZ = lerp(gameCamPosZ, gravCamPosZ, t);
    
    float finalTargetX = lerp(gameTargetX, gravTargetX, t);
    float finalTargetY = lerp(gameTargetY, gravTargetY, t);
    float finalTargetZ = lerp(gameTargetZ, gravTargetZ, t);
    
    float finalUpX = lerp(gameUpX, gravUpX, t);
    float finalUpY = lerp(gameUpY, gravUpY, t);
    float finalUpZ = lerp(gameUpZ, gravUpZ, t);
    
    // Normalize the up vector after interpolation
    float upLen = mySqrt(finalUpX*finalUpX + finalUpY*finalUpY + finalUpZ*finalUpZ);
    if (upLen > 0.0001f) {
        finalUpX /= upLen;
        finalUpY /= upLen;
        finalUpZ /= upLen;
    }
    
    // Apply final interpolated values
    camPos->x = finalCamPosX;
    camPos->y = finalCamPosY;
    camPos->z = finalCamPosZ;
    
    camTarget->x = finalTargetX;
    camTarget->y = finalTargetY;
    camTarget->z = finalTargetZ;
    
    camUp->x = finalUpX;
    camUp->y = finalUpY;
    camUp->z = finalUpZ;
    
    // Recalculate view matrix with interpolated values
    // Forward = normalize(target - position)
    float viewFwdX = finalTargetX - finalCamPosX;
    float viewFwdY = finalTargetY - finalCamPosY;
    float viewFwdZ = finalTargetZ - finalCamPosZ;
    float viewFwdLen = mySqrt(viewFwdX*viewFwdX + viewFwdY*viewFwdY + viewFwdZ*viewFwdZ);
    if (viewFwdLen > 0.0001f) {
        viewFwdX /= viewFwdLen;
        viewFwdY /= viewFwdLen;
        viewFwdZ /= viewFwdLen;
    }
    
    // Right = normalize(forward x up)
    float rightX = viewFwdY * finalUpZ - viewFwdZ * finalUpY;
    float rightY = viewFwdZ * finalUpX - viewFwdX * finalUpZ;
    float rightZ = viewFwdX * finalUpY - viewFwdY * finalUpX;
    float rightLen = mySqrt(rightX*rightX + rightY*rightY + rightZ*rightZ);
    if (rightLen > 0.0001f) {
        rightX /= rightLen;
        rightY /= rightLen;
        rightZ /= rightLen;
    }
    
    // Recalculate up = right x forward (for orthogonality)
    float orthoUpX = rightY * viewFwdZ - rightZ * viewFwdY;
    float orthoUpY = rightZ * viewFwdX - rightX * viewFwdZ;
    float orthoUpZ = rightX * viewFwdY - rightY * viewFwdX;
    
    // Build view matrix (row-major, 3x4)
    viewMtx[0] = rightX;
    viewMtx[1] = rightY;
    viewMtx[2] = rightZ;
    viewMtx[3] = -(rightX * finalCamPosX + rightY * finalCamPosY + rightZ * finalCamPosZ);
    
    viewMtx[4] = orthoUpX;
    viewMtx[5] = orthoUpY;
    viewMtx[6] = orthoUpZ;
    viewMtx[7] = -(orthoUpX * finalCamPosX + orthoUpY * finalCamPosY + orthoUpZ * finalCamPosZ);
    
    viewMtx[8] = -viewFwdX;
    viewMtx[9] = -viewFwdY;
    viewMtx[10] = -viewFwdZ;
    viewMtx[11] = (viewFwdX * finalCamPosX + viewFwdY * finalCamPosY + viewFwdZ * finalCamPosZ);
}

asmFunc CameraUpVectorHook() {
    ASM(
        nofralloc;
        mflr r0;
        stw r0, 0x4(r1);
        stwu r1, -0x30(r1);
        stw r3, 0x8(r1);
        stw r4, 0x10(r1);
        stw r5, 0x14(r1);
        stw r30, 0x1c(r1);
        mr r3, r29;
        bl ModifyCameraUpVector;
        lwz r3, 0x8(r1);
        lwz r4, 0x10(r1);
        lwz r5, 0x14(r1);
        lwz r30, 0x1c(r1);
        addi r1, r1, 0x30;
        lwz r0, 0x4(r1);
        mtlr r0;
        lwz r12, 0x0(r29);
        blr;
    )
}
kmCall(0x805a2abc, CameraUpVectorHook);

} // namespace Race
} // namespace Pulsar