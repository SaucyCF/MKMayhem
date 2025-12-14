#include <Network/SafeGuard.hpp>

namespace Pulsar {
namespace SafeGuard {

namespace {
const u32 MAGIC = '6767';
const u32 DEFAULT_CODE = 'AIMZ';
u32 sRequiredCode = DEFAULT_CODE;
}

void SetRequiredCode(u32 code) {
	if (code == 0) return;
	sRequiredCode = code;
}

u32 GetRequiredCode() { return sRequiredCode; }

void StampUserInfo(Network::ResvInfo::UserInfo& userInfo) {
	userInfo.info[2] = MAGIC;
	userInfo.info[3] = sRequiredCode;
}

bool ValidateUserInfo(const Network::ResvInfo::UserInfo& userInfo) {
	return userInfo.info[2] == MAGIC && userInfo.info[3] == sRequiredCode;
}

} // namespace SafeGuard
} // namespace Pulsar
