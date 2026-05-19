#ifndef _PUL_KO_SUPERRAREPOWER_
#define _PUL_KO_SUPERRAREPOWER_

#include <kamek.hpp>

namespace Pulsar {
namespace KO {

// Check if a player has the Super Rare Power for this race
bool HasSuperRarePower(u8 playerId);

// Check if a player just won the Super Rare Power (for results screen display)
bool JustWonSuperRarePower(u8 playerId);

// Pre-roll Super Rare Power during race (host only)
// Called periodically during race to ensure dice are rolled before results
void PreRollSuperRarePower();

// Apply the pre-rolled results to the display masks (called in ProcessKOs)
void ApplyPreRolledSuperRarePower();

// Clear all Super Rare Power state (called when KO session ends)
void ClearSuperRarePowerState();

// Get the pre-rolled mask for network sync (host sends this during race)
u16 GetPreRolledSuperRarePowerMask();

// Set the pre-rolled mask from network (non-host clients receive during race)
void SetPreRolledSuperRarePowerMask(u16 mask);

// Get the current "just won" mask for network sync
u16 GetJustWonSuperRarePowerMask();

// Set the "just won" mask from network (for non-host clients)
void SetJustWonSuperRarePowerMask(u16 mask);

// Get the next race mask for network sync
u16 GetNextRaceSuperRarePowerMask();

// Set the next race mask from network (for non-host clients)
void SetNextRaceSuperRarePowerMask(u16 mask);

} // namespace KO
} // namespace Pulsar

#endif
