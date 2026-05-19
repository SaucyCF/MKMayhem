#include <Network/SafeGuard.hpp>

namespace Pulsar {
namespace SafeGuard {


static inline u32 BuildSeedA() {
	u32 v = 0x52E1;
	v = (v << 16) | 0x70FB;
	v = (v << 1) | 1;
	return v;
}

static inline u32 BuildSeedB() {
	u32 v = 0x3F8F;
	v = (v << 16) | 0x1E2D;
	v = (v << 1);
	return v;
}

static inline u32 BuildDefaultCode() {
	u32 v = 0x2129;
	v = (v << 1);
	v = (v << 16) | 0x5448;
	return v;
}


static u32 RotLeft(u32 val, u32 n) {
	return (val << n) | (val >> (32 - n));
}

static u32 ScrambleRound(u32 val, u32 key) {
	val ^= key;
	val = RotLeft(val, 13);
	val *= 0x5BD1E995u;
	val ^= val >> 15;
	val *= 0x27D4EB2Fu;
	val ^= val >> 13;
	return val;
}

static u32 Scramble(u32 val, u32 key) {
	val = ScrambleRound(val, key);
	val = ScrambleRound(val, key ^ 0x6C078965u);
	val = ScrambleRound(val, RotLeft(key, 17));
	return val;
}


static void DeriveTokens(u32 code, u32& tokenA, u32& tokenB) {
	const u32 seedA = BuildSeedA();
	const u32 seedB = BuildSeedB();

	u32 a = Scramble(code, seedA);
	u32 b = Scramble(code, seedB);

	a ^= RotLeft(b, 16);
	b += Scramble(a, code);

	a = Scramble(a, b);
	b ^= a * 0x01000193u;

	a += RotLeft(code, 5) ^ RotLeft(b, 23);
	b ^= Scramble(a ^ code, seedA + seedB);

	tokenA = a;
	tokenB = b;
}

u32 sRequiredCode = 0;
u32 sCachedTokenA  = 0;
u32 sCachedTokenB  = 0;
bool sInitialized   = false;

static void RefreshCache() {
	DeriveTokens(sRequiredCode, sCachedTokenA, sCachedTokenB);
}

void SetRequiredCode(u32 code) {
	if (code == 0) return;
	sRequiredCode = code;
	RefreshCache();
}

u32 GetRequiredCode() { return sRequiredCode; }

void Init() {
	if (!sInitialized) {
		sRequiredCode = BuildDefaultCode();
		RefreshCache();
		sInitialized = true;
	}
}

void StampUserInfo(Network::ResvInfo::UserInfo& userInfo) {
	Init();
	userInfo.info[2] = sCachedTokenA;
	userInfo.info[3] = sCachedTokenB;
}

bool ValidateUserInfo(const Network::ResvInfo::UserInfo& userInfo) {
	Init();
	const u32 diffA = userInfo.info[2] ^ sCachedTokenA;
	const u32 diffB = userInfo.info[3] ^ sCachedTokenB;
	return (diffA | diffB) == 0;
}

} // namespace SafeGuard
} // namespace Pulsar
