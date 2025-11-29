#ifndef _PULSAR_SECURITY_SAFEGUARD_HPP_
#define _PULSAR_SECURITY_SAFEGUARD_HPP_
#include <types.hpp>
#include <Network/MatchCommand.hpp>

namespace Pulsar {
namespace SafeGuard {

void SetRequiredCode(u32 code);
u32 GetRequiredCode();

void StampUserInfo(Network::ResvInfo::UserInfo& userInfo);
bool ValidateUserInfo(const Network::ResvInfo::UserInfo& userInfo);

} // namespace SafeGuard
} // namespace Pulsar

#endif
