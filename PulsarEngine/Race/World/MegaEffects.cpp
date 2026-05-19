#include <kamek.hpp>
#include <MarioKartWii/Item/ItemPlayer.hpp>
#include <MarioKartWii/Item/ItemManager.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <PulsarSystem.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <DKW.hpp>

namespace Pulsar {
namespace Race {

// Mega Mushroom Icon - Shows icon while mega effect is active
// ~7 seconds = 430 frames at 60fps
static const u32 MEGA_DURATION_FRAMES = 430;

// Timer for each player (up to 12 players)
static s32 megaIconTimers[12] = {0};

// Check if we're in a valid context for mega effects
static bool ShouldApplyMegaEffects() {
    if (RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_JOINING_REGIONAL) {
        return System::sInstance->IsContext(Pulsar::PULSAR_MAYHEM_WORLD);
    }
    return false;
}

// Hook useMega's useItem call to set the display item and start timer
void MegaMushroomIcon(Item::PlayerInventory* inventory, u32 count) {
    // Call original useItem (removes items from inventory)
    inventory->RemoveItems(count);
    
    if (ShouldApplyMegaEffects()) {
        // Calculate player pointer from inventory pointer (inventory is at offset 0x88)
        Item::Player* player = (Item::Player*)((u8*)inventory - 0x88);
        
        // Set the display item to MEGA_MUSHROOM
        player->roulette.unknown_0x24 = (u32)MEGA_MUSHROOM;
        
        // Start the timer for this player
        u8 playerIdx = player->id;
        if (playerIdx < 12) {
            megaIconTimers[playerIdx] = MEGA_DURATION_FRAMES;
        }
    }
}
kmCall(0x807a9e70, MegaMushroomIcon);

// Update function called every frame to decrement timers and clear icons
void UpdateMegaIconTimers() {
    if (!ShouldApplyMegaEffects()) return;
    
    Item::Manager* mgr = Item::Manager::sInstance;
    if (mgr == nullptr) return;
    
    for (u8 i = 0; i < 12; i++) {
        if (megaIconTimers[i] > 0) {
            megaIconTimers[i]--;
            
            // Timer expired, clear the icon
            if (megaIconTimers[i] <= 0) {
                megaIconTimers[i] = 0;
                
                // Get the item player and clear display
                if (i < mgr->playerCount) {
                    Item::Player* player = &mgr->players[i];
                    if (player != nullptr) {
                        player->roulette.unknown_0x24 = (u32)ITEM_NONE;
                    }
                }
            }
        }
    }
}
RaceFrameHook UpdateTimersHook(UpdateMegaIconTimers);

// Reset timers when race starts
void ResetMegaIconTimers() {
    for (u8 i = 0; i < 12; i++) {
        megaIconTimers[i] = 0;
    }
}
RaceLoadHook ResetTimersHook(ResetMegaIconTimers);

// NOTE: Mega lightning immunity is handled in Boo.cpp (ApplyLightningEffect)
// to avoid duplicate hooks at the same address

}// namespace Race
}// namespace Pulsar
