#include <AutoTrackSelect/ExpWFCMessages.hpp>
#include <Settings/Settings.hpp>
#include <UI/UI.hpp>

namespace Pulsar {
namespace UI {

static u32 GetWorldwideBmgForSetting(DKWSettingWWGamemode setting) {
	switch (setting) {
		case DKWSETTING_WWGAMEMODE_MKDS:
			return BMG_REGULAR_BUTTON;
		case DKWSETTING_WWGAMEMODE_ITEMRAIN:
			return BMG_ITEMRAIN_BUTTON;
		case DKWSETTING_WWGAMEMODE_MAYHEM:
			return BMG_MAYHEM_BUTTON;
		default:
			return BMG_REGIONAL_BUTTON;
	}
}

// Overload for callers that may have integral WW setting values.
static u32 GetWorldwideBmgForSetting(int setting) {
	return GetWorldwideBmgForSetting(static_cast<DKWSettingWWGamemode>(setting));
}

u32 GetWorldwideRegionalBmg() {
	const int setting = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_MISC2, WW_GAMEMODE);
	return GetWorldwideBmgForSetting(setting);
}

void SetRegionalButtonMessage(PushButton& button) {
	button.SetMessage(GetWorldwideRegionalBmg());
}

}  // namespace UI
}  // namespace Pulsar
