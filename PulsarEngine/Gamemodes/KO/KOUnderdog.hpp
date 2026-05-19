#ifndef _PUL_KO_UNDERDOG_
#define _PUL_KO_UNDERDOG_

#include <kamek.hpp>

namespace Pulsar {
namespace KO {

// Check if a player has the underdog bonus (lowest GP points, not eliminated)
// Multiple players can have the bonus if tied for lowest score
bool HasUnderdogBonus(u8 playerId);

// Get the current underdog player mask (bitmask, 0 if none)
u16 GetUnderdogPlayerMask();

// Check if a player will be the underdog for the NEXT race (for results screen display)
bool WillBeUnderdogNextRace(u8 playerId);

// Calculate and store who will be the underdog for the next race
// Must be called BEFORE scores are reset in ProcessKOs
// playersAboutToBeKOd: bitmask of players who are about to be KO'd
void CalculateNextRaceUnderdog(u16 playersAboutToBeKOd);

// Clear all underdog state (called when KO session ends or room is closed)
void ClearUnderdogState();

} // namespace KO
} // namespace Pulsar

#endif
