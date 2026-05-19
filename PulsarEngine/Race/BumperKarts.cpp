#include <kamek.hpp>
#include <PulsarSystem.hpp>
#include <MarioKartWii/Kart/KartPhysics.hpp>
#include <MarioKartWii/Kart/KartLink.hpp>
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <SlotExpansion/CupsConfig.hpp>

namespace Pulsar {
namespace Race {

static const u32 BUMPER_FORCE_SHUFFLE_INTERVAL = 300;
static float bumperForceMultiplier = 6.0f;
static u32 bumperForceTimer = 0;
static u32 bumperForceEpoch = 0;

static void RandomizeBumperForce() {
    if (System::sInstance == nullptr || !System::sInstance->IsContext(PULSAR_MODE_BUMPERKARTS)) return;
    if (bumperForceTimer == 0) {
        const s32 raceNumber = SectionMgr::sInstance->sectionParams->onlineParams.currentRaceNumber;
        const PulsarId courseId = CupsConfig::sInstance->GetWinning();
        const s32 seed = (raceNumber * 7919) ^ (static_cast<s32>(courseId) * 6271) ^ (bumperForceEpoch * 3571);
        Random random(seed);
        bumperForceMultiplier = 1.0f + random.NextFloatLimited(14.0f);
        bumperForceTimer = BUMPER_FORCE_SHUFFLE_INTERVAL;
        bumperForceEpoch++;
    }
    bumperForceTimer--;
}
RaceFrameHook BumperForceRandomizer(RandomizeBumperForce);

static void ResetBumperForceTimer() {
    bumperForceMultiplier = 6.0f;
    bumperForceTimer = 0;
    bumperForceEpoch = 0;
}
RaceLoadHook BumperForceReset(ResetBumperForceTimer);

static void AmplifiedBumpForce(Kart::Physics& physics, Vec3& force) {
    if (System::sInstance != nullptr && System::sInstance->IsContext(PULSAR_MODE_BUMPERKARTS)) {
        force.x *= bumperForceMultiplier;
        force.y *= bumperForceMultiplier;
        force.z *= bumperForceMultiplier;
    }
    physics.AddForce(force);
}
kmCall(0x80571074, AmplifiedBumpForce);

static const Kart::Stats* ForceEqualWeight(Kart::Link& link) {
    const Kart::Stats& stats = link.GetStats();
    if (System::sInstance != nullptr && System::sInstance->IsContext(PULSAR_MODE_BUMPERKARTS)) {
        Kart::Stats& mutableStats = const_cast<Kart::Stats&>(stats);
        mutableStats.weight = 1.0f;
        mutableStats.bumpDeviationLevel = 0.0f;
        mutableStats.weightClass = 1;
    }
    return &stats;
}
kmCall(0x80570e3c, ForceEqualWeight);
kmCall(0x80570e48, ForceEqualWeight);
kmCall(0x80570f20, ForceEqualWeight);
kmCall(0x8057107c, ForceEqualWeight);

} // namespace Race
} // namespace Pulsar
