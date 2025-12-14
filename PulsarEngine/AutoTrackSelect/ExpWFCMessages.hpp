#ifndef _EXPWFCMESSAGES_
#define _EXPWFCMESSAGES_

#include <kamek.hpp>
#include <MarioKartWii/UI/Ctrl/PushButton.hpp>

namespace Pulsar {
namespace UI {

// Returns the BMG id to display on the regional button based on the WW gamemode setting.
u32 GetWorldwideRegionalBmg();

// Applies the WW-aware BMG to the provided regional button.
void SetRegionalButtonMessage(PushButton& button);

}  // namespace UI
}  // namespace Pulsar

#endif
