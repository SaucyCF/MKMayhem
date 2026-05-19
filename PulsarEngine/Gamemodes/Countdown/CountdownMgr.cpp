#include <kamek.hpp>
#include <PulsarSystem.hpp>
#include <Settings/Settings.hpp>
#include <Gamemodes/Countdown/CountdownMgr.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <MarioKartWii/Race/RaceData.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/Kart/KartPlayer.hpp>
#include <MarioKartWii/Item/Obj/ItemObj.hpp>
#include <MarioKartWii/UI/Ctrl/CtrlRace/CtrlRaceLap.hpp>
#include <MarioKartWii/UI/Ctrl/Animation.hpp>
#include <MarioKartWii/Kart/KartDamage.hpp>
#include <MarioKartWii/Kart/KartLink.hpp>
#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/Kart/KartBody.hpp>
#include <MarioKartWii/Kart/KartPhysics.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <MarioKartWii/Kart/KartSub.hpp>
#include <MarioKartWii/3D/Camera/CameraMgr.hpp>
#include <MarioKartWii/3D/Camera/RaceCamera.hpp>
#include <MarioKartWii/Driver/DriverManager.hpp>
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <core/rvl/PAD.hpp>
#include <core/rvl/WPAD.hpp>

namespace Pulsar {
namespace Countdown {

struct TrackStartTime {
    u32 trackCrc32;
    u32 startSeconds;
};

static const TrackStartTime kTrackStartTimes[] = {
    { 0xad2257dc, 30 }, // GCN Baby Park
    { 0u, 0u },  // sentinel — keep last
};

static u8 sLastCollisionOwnerByVictim[12] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static u32 sLastCollisionFrameByVictim[12] = {0};
static u32 sLastTalliedItemsHitCount[12] = {
    0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
    0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
    0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu
};
static bool sLocalTimedOutLastFrame[12] = {false};
static const u32 kCollisionOwnerMemoryFrames = 180;

struct CountdownSpectateState {
    bool active;
    u8 targetPlayerId;
    bool manualTarget;
    u32 lastFocusFrame;
};

static CountdownSpectateState sCountdownSpectate = {false, 0xFF, false, 0};

static bool IsInvalidPTR(const void* ptr) {
    const u32 addr = reinterpret_cast<u32>(ptr);
    if (ptr == 0) return true;
    if ((addr & 0x3u) != 0) return true;
    // Countdown only expects game memory pointers; reject small integer/flag junk.
    if ((addr & 0x80000000u) == 0) return true;
    return false;
}

static RaceinfoPlayer* GetValidRaceinfoPlayer(RaceinfoPlayer** players, u8 playerId) {
    if (IsInvalidPTR(players) || playerId >= 12) return 0;
    RaceinfoPlayer* player = players[playerId];
    if (IsInvalidPTR(player)) return 0;
    if (player->id != playerId) return 0;
    return player;
}

static bool CanCallEndRace(const RaceinfoPlayer* player, u8 expectedPlayerId) {
    if (IsInvalidPTR(player)) return false;
    if (player->id != expectedPlayerId) return false;
    if (player->raceFinishTime == 0) return false;
    if (IsInvalidPTR(player->raceFinishTime)) return false;
    return true;
}

static u32 TimerToMs(const Timer& timer) {
    return static_cast<u32>(timer.minutes) * 60000u
         + static_cast<u32>(timer.seconds) * 1000u
         + static_cast<u32>(timer.milliseconds);
}

static void SetTimerFromMs(Timer& timer, u32 totalMs) {
    const u32 maxMs = (999u * 60000u) + (59u * 1000u) + 999u;
    if (totalMs > maxMs) totalMs = maxMs;
    timer.minutes = static_cast<u16>(totalMs / 60000u);
    totalMs %= 60000u;
    timer.seconds = static_cast<u8>(totalMs / 1000u);
    timer.milliseconds = static_cast<u16>(totalMs % 1000u);
    timer.SetActive(true);
}

static Timer MakeLeaderPlus30sCutoff(Raceinfo& raceinfo, const u8* ordered, u8 orderedCount, RaceinfoPlayer** players) {
    Timer base(false);
    bool foundLeaderFinish = false;

    for (u8 i = 0; i < orderedCount; ++i) {
        const u8 pid = ordered[i];
        if (pid >= 12) continue;
        RaceinfoPlayer* p = GetValidRaceinfoPlayer(players, pid);
        if (p == 0) continue;
        if (p->raceFinishTime == 0 || IsInvalidPTR(p->raceFinishTime)) continue;
        if (!p->raceFinishTime->isActive) continue;
        base = *p->raceFinishTime;
        foundLeaderFinish = true;
        break;
    }

    if (!foundLeaderFinish) {
        raceinfo.CloneTimer(&base);
    }

    const u32 cutoffMs = TimerToMs(base) + 30000u;
    SetTimerFromMs(base, cutoffMs);
    return base;
}

static const u8* GetValidPositionOrder(const Raceinfo* raceinfo) {
    if (raceinfo == 0) return 0;
    const u8* order = raceinfo->playerIdInEachPosition;
    if (IsInvalidPTR(order)) return 0;
    u8 validCount = 0;
    for (u8 i = 0; i < 12; ++i) {
        const u8 pid = order[i];
        if (pid < 12) {
            ++validCount;
            continue;
        }
        if (pid != 0xFF) return 0;
    }
    return (validCount > 0) ? order : 0;
}

static void ResetCountdownSpectateState() {
    sCountdownSpectate.active = false;
    sCountdownSpectate.targetPlayerId = 0xFF;
    sCountdownSpectate.manualTarget = false;
    sCountdownSpectate.lastFocusFrame = 0;
}

static s32 FindCountdownSpectateIndex(const u8* order, u8 count, u8 playerId) {
    if (order == 0) return -1;
    for (u8 i = 0; i < count; ++i) {
        if (order[i] == playerId) return static_cast<s32>(i);
    }
    return -1;
}

static bool FocusLocalCameraOnPlayer(u8 targetPlayerId) {
    if (targetPlayerId >= 12) return false;
    RaceCameraMgr* camMgr = RaceCameraMgr::sInstance;
    if (camMgr == 0) return false;
    if (IsInvalidPTR(camMgr)) return false;
    if (camMgr->cameras == 0) return false;
    if (IsInvalidPTR(camMgr->cameras)) return false;
    const u32 camCount = camMgr->cameraCount;
    if (camCount == 0 || camCount > 4) return false;

    u8 targetCamIdx = 0xFF;
    for (u32 i = 0; i < camCount; ++i) {
        RaceCamera* cam = camMgr->cameras[i];
        if (cam == 0 || IsInvalidPTR(cam)) continue;
        const u8 pid = cam->playerId;
        if (pid >= 12 && pid != 0xFF) continue;
        if (cam->playerId == targetPlayerId) {
            targetCamIdx = static_cast<u8>(i);
            break;
        }
    }

    if (targetCamIdx != 0xFF) {
        DriverMgr::ChangeFocusedPlayer(targetCamIdx);
        RaceCameraMgr::ChangeFocusedPlayer(targetCamIdx);
        return true;
    }

    const u32 currentIdx = (camMgr->focusedPlayerIdx < camCount) ? camMgr->focusedPlayerIdx : 0;
    RaceCamera* currentCam = camMgr->cameras[currentIdx];
    if (currentCam == 0 || IsInvalidPTR(currentCam)) return false;
    const u8 pid = currentCam->playerId;
    if (pid >= 12 && pid != 0xFF) return false;

    // LapKO-style fallback: repoint current camera when no per-player camera exists.
    if (currentCam->playerId != targetPlayerId) {
        currentCam->playerId = targetPlayerId;
    }
    DriverMgr::ChangeFocusedPlayer(static_cast<u8>(currentIdx));
    return true;
}

static bool IsCountdownActive(const System* system) {
    if (system == nullptr) return false;
    if (system->IsContext(PULSAR_MODE_LAPKO)) return false;
    if (system->IsContext(PULSAR_MODE_COUNTDOWN)) return true;
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    if (controller == nullptr) return false;
    if (controller->roomType != RKNet::ROOMTYPE_FROOM_NONHOST
            && controller->roomType != RKNet::ROOMTYPE_FROOM_HOST
            && controller->roomType != RKNet::ROOMTYPE_NONE) {
        return false;
    }
    return false;
}

static u8 ResolveItemOwnerId(const Item::Obj& obj) {
    u8 ownerId = obj.playerUsedItemId;
    if (ownerId < 12) return ownerId;
    // Hit-dragged/circle events encode owner in the high nibble of eventBitfield.
    ownerId = static_cast<u8>((obj.eventBitfield >> 8) & 0xF);
    return ownerId;
}

static u32 ResolveStartSeconds() {
    if (CupsConfig::sInstance == nullptr) return Mgr::DEFAULT_START_SECONDS;
    const PulsarId pid = CupsConfig::sInstance->GetWinning();
    const u32 crc = static_cast<u32>(CupsConfig::sInstance->GetCRC32(pid));
    if (crc == 0) return Mgr::DEFAULT_START_SECONDS;
    for (const TrackStartTime* p = kTrackStartTimes; p->trackCrc32 != 0u; ++p) {
        if (p->trackCrc32 == crc) return p->startSeconds;
    }
    return Mgr::DEFAULT_START_SECONDS;
}

u8 Mgr::GetLocalPlayerId() {
    Racedata* rd = Racedata::sInstance;
    if (rd == nullptr) return 0xFF;
    const RacedataScenario& scenario = rd->menusScenario;
    if (scenario.localPlayerCount == 0) return 0xFF;
    return scenario.settings.hudPlayerIds[0];
}

static u8 GetOfflinePlayerCount() {
    const Racedata* rd = Racedata::sInstance;
    if (rd == nullptr) return 12;
    u8 count = rd->racesScenario.playerCount;
    if (count > 12) count = 12;
    return count;
}

static u8 GetRacePlayerCount() {
    const Raceinfo* ri = Raceinfo::sInstance;
    if (ri == nullptr || ri->players == nullptr) return GetOfflinePlayerCount();
    u8 count = 0;
    for (u8 i = 0; i < 12; ++i) {
        if (ri->players[i] != nullptr) ++count;
    }
    if (count == 0) return GetOfflinePlayerCount();
    return count;
}

static u16 BuildParticipantMask(const Raceinfo* raceinfo) {
    u16 mask = 0;
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const bool isOnline = controller != nullptr
        && (controller->roomType == RKNet::ROOMTYPE_FROOM_HOST
            || controller->roomType == RKNet::ROOMTYPE_FROOM_NONHOST);

    if (isOnline) {
        const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];

        // Primary source online: mapped playerId -> aid that is still connected.
        for (u8 pid = 0; pid < 12; ++pid) {
            const u8 aid = controller->aidsBelongingToPlayerIds[pid];
            if (aid >= 12) continue;
            if ((sub.availableAids & (1u << aid)) == 0) continue;
            mask = static_cast<u16>(mask | static_cast<u16>(1u << pid));
        }

        // Fallback online: contiguous ids up to RKNet's room player count.
        u8 onlineCount = 0;
        onlineCount = static_cast<u8>(sub.playerCount);
        if (onlineCount == 0) {
            const System* sys = System::sInstance;
            if (sys != nullptr) onlineCount = sys->nonTTGhostPlayersCount;
        }
        if (onlineCount > 12) onlineCount = 12;
        for (u8 pid = 0; pid < onlineCount; ++pid) {
            mask = static_cast<u16>(mask | static_cast<u16>(1u << pid));
        }
    }

    if (raceinfo != nullptr) {
        const u8* order = GetValidPositionOrder(raceinfo);
        if (order != nullptr) {
            for (u8 i = 0; i < 12; ++i) {
                const u8 pid = order[i];
                if (pid >= 12) continue;
                mask = static_cast<u16>(mask | static_cast<u16>(1u << pid));
            }
        }

        if (!IsInvalidPTR(raceinfo->players)) {
            for (u8 pid = 0; pid < 12; ++pid) {
                if (GetValidRaceinfoPlayer(raceinfo->players, pid) != 0) {
                    mask = static_cast<u16>(mask | static_cast<u16>(1u << pid));
                }
            }
        }
    }

    if (!isOnline) {
        // Offline floor: include configured scenario slots.
        const Racedata* rd = Racedata::sInstance;
        if (rd != nullptr) {
            u8 count = rd->racesScenario.playerCount;
            if (count > 12) count = 12;
            for (u8 i = 0; i < count; ++i) {
                mask = static_cast<u16>(mask | static_cast<u16>(1u << i));
            }
        }
    }
    return mask;
}

static bool IsOfflineLocalPlayer(u8 playerId) {
    if (playerId >= 12) return false;
    const Racedata* rd = Racedata::sInstance;
    if (rd == nullptr) return false;
    const RacedataScenario& scenario = rd->menusScenario;
    for (u8 i = 0; i < scenario.localPlayerCount && i < 4; ++i) {
        if (scenario.settings.hudPlayerIds[i] == playerId) return true;
    }
    return false;
}

static bool IsOfflineCountdownRace() {
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    return controller == nullptr || controller->roomType == RKNet::ROOMTYPE_NONE;
}

static bool IsOnlineCountdownRace() {
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    if (controller == nullptr) return false;
    return controller->roomType == RKNet::ROOMTYPE_FROOM_HOST
        || controller->roomType == RKNet::ROOMTYPE_FROOM_NONHOST;
}

static u32 ReadPlayerItemsHitCount(u8 playerId) {
    if (playerId >= 12) return 0xFFFFFFFFu;
    Kart::Manager* kartMgr = Kart::Manager::sInstance;
    if (kartMgr == nullptr || kartMgr->players == nullptr) return 0xFFFFFFFFu;
    Kart::Player* player = kartMgr->players[playerId];
    if (player == nullptr || IsInvalidPTR(player)) return 0xFFFFFFFFu;
    Kart::Values* values = player->values;
    if (values == nullptr || IsInvalidPTR(values)) return 0xFFFFFFFFu;
    Kart::Values::Tallies* tallies = values->tallies;
    if (tallies == nullptr || IsInvalidPTR(tallies)) return 0xFFFFFFFFu;
    return tallies->itemsHitCount;
}

static u8 BuildLocalPlayerList(u8* outIds, u8 capacity) {
    if (outIds == 0 || capacity == 0) return 0;
    const Racedata* rd = Racedata::sInstance;
    if (rd == nullptr) return 0;

    const RacedataScenario& scenario = rd->menusScenario;
    u8 count = 0;
    for (u8 i = 0; i < scenario.localPlayerCount && i < 4 && count < capacity; ++i) {
        const u8 pid = scenario.settings.hudPlayerIds[i];
        if (pid >= 12) continue;
        bool already = false;
        for (u8 j = 0; j < count; ++j) {
            if (outIds[j] == pid) {
                already = true;
                break;
            }
        }
        if (!already) outIds[count++] = pid;
    }

    if (count == 0) {
        const u8 fallback = Mgr::GetLocalPlayerId();
        if (fallback < 12) outIds[count++] = fallback;
    }
    return count;
}

static bool IsOfflineOutOfTimeCpu(u8 playerId) {
    if (playerId >= 12 || IsOfflineLocalPlayer(playerId)) return false;
    if (!IsCountdownActive(System::sInstance)) return false;

    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const bool isOffline = (controller == nullptr) || (controller->roomType == RKNet::ROOMTYPE_NONE);
    if (!isOffline) return false;

    System* sys = System::sInstance;
    if (sys == nullptr || sys->countdownMgr == nullptr) return false;
    return (sys->countdownMgr->GetTicksRemaining(playerId) == 0)
        || sys->countdownMgr->IsOfflineExpired(playerId);
}

static void ClearOfflineExpiredCpuHitState(Kart::Player* kart) {
    if (kart == nullptr) return;
    Kart::Status* status = kart->pointers.kartStatus;
    if (status != nullptr) {
        status->bitfield1 &= ~0x1u;                                // clear "hit by item/object" latch
        status->bitfield2 &= ~(0x4u | 0x10u | 0x80u | 0x10000u);  // clear bump/rotation/shock/crushed states
    }
    if (kart->pointers.kartMovement != nullptr) {
        kart->pointers.kartMovement->shockTimer = 0;
        kart->pointers.kartMovement->crushTimer = 0;
    }
    if (kart->pointers.kartDamage != nullptr) {
        kart->pointers.kartDamage->curDamageCounter = 0;
        kart->pointers.kartDamage->totalDamageRot = 0.0f;
    }
}

static void UpdateCountdownSpectatorView() {
    System* sys = System::sInstance;
    if (sys == nullptr || sys->countdownMgr == nullptr) return;
    if (!IsCountdownActive(sys)) return;

    Raceinfo* raceinfo = Raceinfo::sInstance;
    if (raceinfo == nullptr || raceinfo->stage < RACESTAGE_RACE || raceinfo->stage >= RACESTAGE_FINISHED) return;
    RaceinfoPlayer** players = raceinfo->players;
    if (IsInvalidPTR(players)) {
        ResetCountdownSpectateState();
        return;
    }

    u8 activeOrder[12];
    u8 activeCount = 0;
    const u8* playerIdInEachPosition = GetValidPositionOrder(raceinfo);
    if (playerIdInEachPosition != nullptr) {
        for (u8 pos = 0; pos < 12 && activeCount < 12; ++pos) {
            const u8 pid = playerIdInEachPosition[pos];
            if (pid >= 12) continue;
            if (sys->countdownMgr->GetTicksRemaining(pid) == 0) continue;
            if (GetValidRaceinfoPlayer(players, pid) == 0) continue;
            bool already = false;
            for (u8 i = 0; i < activeCount; ++i) {
                if (activeOrder[i] == pid) {
                    already = true;
                    break;
                }
            }
            if (!already) activeOrder[activeCount++] = pid;
        }
    }
    for (u8 pid = 0; pid < 12 && activeCount < 12; ++pid) {
        if (sys->countdownMgr->GetTicksRemaining(pid) == 0) continue;
        if (GetValidRaceinfoPlayer(players, pid) == 0) continue;
        bool already = false;
        for (u8 i = 0; i < activeCount; ++i) {
            if (activeOrder[i] == pid) {
                already = true;
                break;
            }
        }
        if (!already) activeOrder[activeCount++] = pid;
    }

    bool anyLocalExpired = false;
    bool localJustExpired = false;
    u8 localIds[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    const u8 localCount = BuildLocalPlayerList(localIds, 4);
    for (u8 i = 0; i < localCount; ++i) {
        const u8 localId = localIds[i];
        const bool isExpired = (sys->countdownMgr->GetTicksRemaining(localId) == 0);
        if (isExpired && !sLocalTimedOutLastFrame[localId]) localJustExpired = true;  // timer just flipped to normal
        sLocalTimedOutLastFrame[localId] = isExpired;
        if (isExpired) anyLocalExpired = true;
    }
    if (!anyLocalExpired) {
        ResetCountdownSpectateState();
        return;
    }

    if (activeCount == 0) {
        ResetCountdownSpectateState();
        return;
    }
    const u8 previousTarget = sCountdownSpectate.targetPlayerId;

    if (localJustExpired) {
        sCountdownSpectate.manualTarget = false;
        sCountdownSpectate.targetPlayerId = activeOrder[0];
        if (FocusLocalCameraOnPlayer(sCountdownSpectate.targetPlayerId)) {
            sCountdownSpectate.active = true;
            sCountdownSpectate.lastFocusFrame = raceinfo->raceFrames;
        }
    }

    bool advanceForward = false;
    bool advanceBackward = false;
    SectionMgr* sectionMgr = SectionMgr::sInstance;
    if (sectionMgr != nullptr) {
        for (u8 hudSlot = 0; hudSlot < 4; ++hudSlot) {
            Input::RealControllerHolder* holder = sectionMgr->pad.padInfos[hudSlot].controllerHolder;
            if (holder == nullptr || holder->curController == nullptr) continue;
            const u16 current = holder->inputStates[0].buttonRaw;
            const u16 previous = holder->inputStates[1].buttonRaw;
            const u16 newInputs = static_cast<u16>(current & static_cast<u16>(~previous));
            if (newInputs == 0) continue;

            const ControllerType type = holder->curController->GetType();
            switch (type) {
                case WHEEL:
                case NUNCHUCK:
                    if ((newInputs & WPAD::WPAD_BUTTON_A) != 0) advanceForward = true;
                    if ((newInputs & WPAD::WPAD_BUTTON_B) != 0) advanceBackward = true;
                    break;
                case CLASSIC:
                    if ((newInputs & WPAD::WPAD_CL_BUTTON_A) != 0) advanceForward = true;
                    if ((newInputs & WPAD::WPAD_CL_BUTTON_B) != 0) advanceBackward = true;
                    break;
                case GCN:
                    if ((newInputs & PAD::PAD_BUTTON_A) != 0) advanceForward = true;
                    if ((newInputs & PAD::PAD_BUTTON_B) != 0) advanceBackward = true;
                    break;
                default:
                    if ((newInputs & PAD::PAD_BUTTON_A) != 0) advanceForward = true;
                    if ((newInputs & PAD::PAD_BUTTON_B) != 0) advanceBackward = true;
                    if ((newInputs & WPAD::WPAD_BUTTON_A) != 0) advanceForward = true;
                    if ((newInputs & WPAD::WPAD_BUTTON_B) != 0) advanceBackward = true;
                    if ((newInputs & WPAD::WPAD_CL_BUTTON_A) != 0) advanceForward = true;
                    if ((newInputs & WPAD::WPAD_CL_BUTTON_B) != 0) advanceBackward = true;
                    break;
            }
        }
    }

    if (!sCountdownSpectate.manualTarget) {
        sCountdownSpectate.targetPlayerId = activeOrder[0];
    }
    if (advanceForward || advanceBackward) {
        s32 idx = FindCountdownSpectateIndex(activeOrder, activeCount, sCountdownSpectate.targetPlayerId);
        if (idx < 0) idx = 0;
        if (advanceForward) idx = (idx + 1) % activeCount;
        else idx = (idx + activeCount - 1) % activeCount;
        sCountdownSpectate.targetPlayerId = activeOrder[idx];
        sCountdownSpectate.manualTarget = true;
    }

    if (FindCountdownSpectateIndex(activeOrder, activeCount, sCountdownSpectate.targetPlayerId) < 0) {
        sCountdownSpectate.targetPlayerId = activeOrder[0];
        if (activeCount == 0) sCountdownSpectate.manualTarget = false;
    }

    const u32 frame = raceinfo->raceFrames;
    const bool targetChanged = (!sCountdownSpectate.active) || (previousTarget != sCountdownSpectate.targetPlayerId);
    const bool shouldRefreshFocus = targetChanged || (frame - sCountdownSpectate.lastFocusFrame >= 20);
    if (!shouldRefreshFocus) return;

    if (FocusLocalCameraOnPlayer(sCountdownSpectate.targetPlayerId)) {
        sCountdownSpectate.active = true;
        sCountdownSpectate.lastFocusFrame = frame;
    }
}

Mgr::Mgr() : startFrames(DEFAULT_START_FRAMES), localSpectating(false) {
    for (int i = 0; i < 12; ++i) {
        this->ticksRemaining[i] = DEFAULT_START_FRAMES;
        this->overtimeFrames[i] = 0;
        this->score[i] = 0;
        this->lastScoredItemIdx[i] = NO_LAST_ITEM;
        this->lastScoredFrame[i] = 0;
        for (int v = 0; v < 12; ++v) {
            this->lastScoredVictimItemIdx[i][v] = NO_LAST_ITEM;
            this->lastScoredVictimFrame[i][v] = 0;
        }
        this->raceEnded[i] = false;
        this->offlineExpired[i] = false;
        this->offlineElimOrder[i] = 0xFF;
        this->onlineExpired[i] = false;
        this->onlineElimOrder[i] = 0xFF;
        sLastTalliedItemsHitCount[i] = 0xFFFFFFFFu;
    }
    this->offlineElimCount = 0;
    this->onlineElimCount = 0;
}

void Mgr::OnRaceLoad() {
    const u32 startSeconds = ResolveStartSeconds();
    this->startFrames = startSeconds * FRAMES_PER_SECOND;
    for (int i = 0; i < 12; ++i) {
        this->ticksRemaining[i] = this->startFrames;
        this->overtimeFrames[i] = 0;
        this->score[i] = 0;
        this->lastScoredItemIdx[i] = NO_LAST_ITEM;
        this->lastScoredFrame[i] = 0;
        for (int v = 0; v < 12; ++v) {
            this->lastScoredVictimItemIdx[i][v] = NO_LAST_ITEM;
            this->lastScoredVictimFrame[i][v] = 0;
        }
        this->raceEnded[i] = false;
        this->offlineExpired[i] = false;
        this->offlineElimOrder[i] = 0xFF;
        this->onlineExpired[i] = false;
        this->onlineElimOrder[i] = 0xFF;
        sLastCollisionOwnerByVictim[i] = 0xFF;
        sLastCollisionFrameByVictim[i] = 0;
        sLastTalliedItemsHitCount[i] = 0xFFFFFFFFu;
        sLocalTimedOutLastFrame[i] = false;
    }
    this->offlineElimCount = 0;
    this->onlineElimCount = 0;
    this->localSpectating = false;
    ResetCountdownSpectateState();
    SeedReversedTimer(startSeconds);
}

void Mgr::FreezeOfflineExpiredCpu(u8 playerId) {
    if (playerId >= 12) return;
    Kart::Manager* kartMgr = Kart::Manager::sInstance;
    if (kartMgr == nullptr || kartMgr->players == nullptr) return;
    Kart::Player* kart = kartMgr->players[playerId];
    if (kart == nullptr) return;
    if (kart->pointers.kartStatus == nullptr) return;
    ClearOfflineExpiredCpuHitState(kart);

    const u32 kInBulletBit = 0x08000000u;
    kart->pointers.kartStatus->stickX = 0.0f;
    kart->pointers.kartStatus->stickY = 0.0f;
    kart->pointers.kartStatus->bitfield0 &= ~(0x2u | 0x4u | 0x8u | 0x10u | 0x2000u | 0x1000000u);
    kart->pointers.kartStatus->bitfield0 |= 0x1u;  // accelerate
    kart->pointers.kartStatus->bitfield1 &= ~0x2u;         // TRIGGER_RESPAWN
    kart->pointers.kartStatus->bitfield2 &= ~kInBulletBit; // force out of bullet state
    kart->pointers.kartStatus->bitfield2 &= ~0x0008E000u;  // clear vanished + post-respawn flags
    kart->pointers.kartStatus->bitfield2 &= ~0x00040000u;  // keep collisions enabled
    kart->pointers.kartStatus->bitfield4 &= ~0x00001000u;  // keep collisions enabled
    kart->pointers.kartStatus->bitfield4 &= ~0x00180000u;  // clear OOB transition flags
    kart->ToggleVisible(true, false, true, true);  // keep kart/driver visible, hide bullet model

    // Hard-stop motion while keeping forward-driving state.
    if (kart->pointers.kartMovement != nullptr) {
        Kart::Movement* mv = kart->pointers.kartMovement;
        mv->engineSpeed = 0.0f;
        mv->lastSpeed = 0.0f;
        mv->acceleration = 0.0f;
        mv->softSpeedLimit = 0.0f;
        mv->hardSpeedLimit = 0.0f;
        mv->speedRatio = 0.0f;
        mv->speedRatioCapped = 0.0f;
        mv->drivingDirection = 0;      // forward
        mv->backwardsAllowCounter = 0x7fff;
    }
    if (kart->pointers.kartBody != nullptr && kart->pointers.kartBody->kartPhysicsHolder != nullptr &&
        kart->pointers.kartBody->kartPhysicsHolder->physics != nullptr) {
        Kart::Physics* ph = kart->pointers.kartBody->kartPhysicsHolder->physics;
        ph->speed0 = Vec3(0.0f, 0.0f, 0.0f);
        ph->speed1Adj = Vec3(0.0f, 0.0f, 0.0f);
        ph->speed2 = Vec3(0.0f, 0.0f, 0.0f);
        ph->speed3 = Vec3(0.0f, 0.0f, 0.0f);
        ph->speed = Vec3(0.0f, 0.0f, 0.0f);
        ph->engineSpeed = Vec3(0.0f, 0.0f, 0.0f);
        ph->speedNorm = 0.0f;
    }
}

void Mgr::FinalizeOfflinePlacementsAndEnd() {
    Raceinfo* raceinfo = Raceinfo::sInstance;
    if (raceinfo == nullptr || raceinfo->players == nullptr) return;
    if ((reinterpret_cast<u32>(raceinfo->players) & 0x3u) != 0) return;
    RaceinfoPlayer** players = raceinfo->players;
    const u8* playerIdInEachPosition = GetValidPositionOrder(raceinfo);

    Timer now(false);
    raceinfo->CloneTimer(&now);
    now.SetActive(true);

    const u8 total = GetOfflinePlayerCount();

    // Finalize everyone by current standings at race end.
    if (playerIdInEachPosition != nullptr) {
        for (u8 pos = 0; pos < total; ++pos) {
            const u8 pid = playerIdInEachPosition[pos];
            if (pid >= 12 || this->raceEnded[pid]) continue;
            RaceinfoPlayer* p = GetValidRaceinfoPlayer(players, pid);
            if (!CanCallEndRace(p, pid)) continue;
            const Timer* commitTime = &now;
            if (p->raceFinishTime != nullptr && !IsInvalidPTR(p->raceFinishTime) && p->raceFinishTime->isActive) {
                commitTime = p->raceFinishTime;
            }
            p->EndRace(*commitTime, false, 0);
            raceinfo->EndPlayerRace(pid);
            this->raceEnded[pid] = true;
        }
    } else {
        for (u8 pid = 0; pid < total; ++pid) {
            if (this->raceEnded[pid]) continue;
            RaceinfoPlayer* p = GetValidRaceinfoPlayer(players, pid);
            if (!CanCallEndRace(p, pid)) continue;
            const Timer* commitTime = &now;
            if (p->raceFinishTime != nullptr && !IsInvalidPTR(p->raceFinishTime) && p->raceFinishTime->isActive) {
                commitTime = p->raceFinishTime;
            }
            p->EndRace(*commitTime, false, 0);
            raceinfo->EndPlayerRace(pid);
            this->raceEnded[pid] = true;
        }
    }
}

void Mgr::FinalizeOnlinePlacementsAndEnd() {
    // ONLINE finalize.
    // Final placements are whatever vanilla's playerIdInEachPosition says at
    // finalize-time — i.e. natural race progress, exactly like a normal race.
    // The only thing the gamemode owns is the *timing* of when EndRace is
    // called: we wait until every participant has timed out, then end the
    // race for everyone in current race-position order so the post-race UI
    // displays the natural rankings.
    Raceinfo* raceinfo = Raceinfo::sInstance;
    if (raceinfo == nullptr || raceinfo->players == nullptr) return;
    if ((reinterpret_cast<u32>(raceinfo->players) & 0x3u) != 0) return;
    RaceinfoPlayer** players = raceinfo->players;
    const u8* playerIdInEachPosition = GetValidPositionOrder(raceinfo);
    const u16 participantMask = BuildParticipantMask(raceinfo);

    u8 ordered[12];
    u8 orderedCount = 0;
    // Pass 1: walk current race positions 1..12. First entry = whoever's in
    // 1st right now, etc. Vanilla's finishedPlayerCount increments per call,
    // so position-1's EndRace happens first and they end up 1st on the
    // results screen.
    if (playerIdInEachPosition != nullptr) {
        for (u8 pos = 0; pos < 12 && orderedCount < 12; ++pos) {
            const u8 pid = playerIdInEachPosition[pos];
            if (pid >= 12) continue;
            if ((participantMask & static_cast<u16>(1u << pid)) == 0) continue;
            if (this->raceEnded[pid]) continue;
            ordered[orderedCount++] = pid;
        }
    }
    // Pass 2: any participant not present in playerIdInEachPosition (very
    // rare, network corner case) still gets EndRace called so no one is
    // left hanging.
    for (u8 pid = 0; pid < 12 && orderedCount < 12; ++pid) {
        if ((participantMask & static_cast<u16>(1u << pid)) == 0) continue;
        if (this->raceEnded[pid]) continue;
        bool already = false;
        for (u8 j = 0; j < orderedCount; ++j) {
            if (ordered[j] == pid) { already = true; break; }
        }
        if (!already) ordered[orderedCount++] = pid;
    }
    if (orderedCount == 0) return;

    // Shared finish time so the call order itself (= current position order)
    // is what breaks ties in vanilla's results UI.
    Timer now(false);
    raceinfo->CloneTimer(&now);
    now.SetActive(true);

    for (u8 i = 0; i < orderedCount; ++i) {
        const u8 pid = ordered[i];
        if ((participantMask & static_cast<u16>(1u << pid)) == 0) continue;
        if (this->raceEnded[pid]) continue;
        RaceinfoPlayer* p = GetValidRaceinfoPlayer(players, pid);
        if (!CanCallEndRace(p, pid)) continue;
        p->EndRace(now, false, 0);
        this->raceEnded[pid] = true;
    }
}

void Mgr::PackForNetwork(u8* outScores, u32* outTicks) const {
    outScores[0] = 0;
    outScores[1] = 0;
    outTicks[0]  = 0;
    outTicks[1]  = 0;
    const Racedata* rd = Racedata::sInstance;
    if (rd == nullptr) return;
    const RacedataScenario& scenario = rd->menusScenario;
    const u8 localCount = (scenario.localPlayerCount > 2) ? 2 : scenario.localPlayerCount;
    for (u8 slot = 0; slot < localCount; ++slot) {
        const u8 pid = scenario.settings.hudPlayerIds[slot];
        if (pid >= 12) continue;
        outScores[slot] = this->score[pid];
        outTicks[slot]  = this->ticksRemaining[pid];
    }
}

void Mgr::UnpackFromNetwork(u8 senderAid, const u8* inScores, const u32* inTicks) {
    if (senderAid >= 12 || inScores == nullptr || inTicks == nullptr) return;
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    if (controller == nullptr) return;
    u8 slot = 0;
    for (u8 pid = 0; pid < 12 && slot < 2; ++pid) {
        if (controller->aidsBelongingToPlayerIds[pid] != senderAid) continue;
        if (Racedata::sInstance != nullptr) {
            const RacedataScenario& scenario = Racedata::sInstance->menusScenario;
            bool isLocal = false;
            for (u8 i = 0; i < scenario.localPlayerCount && i < 4; ++i) {
                if (scenario.settings.hudPlayerIds[i] == pid) { isLocal = true; break; }
            }
            if (isLocal) { ++slot; continue; }
        }
        if (inScores[slot] <= MAX_SCORE) this->score[pid] = inScores[slot];
        this->ticksRemaining[pid] = inTicks[slot];
        ++slot;
    }
}

void Mgr::SeedReversedTimer(u32 totalSeconds) {
    Raceinfo* raceInfo = Raceinfo::sInstance;
    if (raceInfo == nullptr || raceInfo->timerMgr == nullptr) return;
    RaceTimerMgr* tm = raceInfo->timerMgr;
    const u16 mins = (u16)(totalSeconds / 60);
    const u8  secs = (u8)(totalSeconds % 60);
    tm->isTimerReversed = true;
    tm->hasRaceTimeRanOut = false;
    // Prevent vanilla timeout flow (99:59.999) from ending Countdown races.
    tm->raceDurationMs = 0x7fffffffu;
    tm->timers[2].minutes = mins;
    tm->timers[2].seconds = secs;
    tm->timers[2].milliseconds = 0;
    tm->timers[2].isActive = true;
    tm->timers[1].minutes = mins;
    tm->timers[1].seconds = secs;
    tm->timers[1].milliseconds = 0;
    tm->timers[1].isActive = true;
}

void Mgr::OnItemHit(u8 ownerPlayerId, u8 victimPlayerId, u16 itemInstanceIdx) {
    (void)victimPlayerId;
    (void)itemInstanceIdx;
    this->OnSoloHit(ownerPlayerId, 0);
}

void Mgr::OnSoloHit(u8 ownerPlayerId, u16 itemInstanceIdx) {
    (void)itemInstanceIdx;
    if (ownerPlayerId >= 12) return;
    if (this->ticksRemaining[ownerPlayerId] == 0) return;
    if (this->score[ownerPlayerId] >= MAX_SCORE) return;
    this->score[ownerPlayerId]++;
    u32 newTicks = this->ticksRemaining[ownerPlayerId] + TIME_BONUS_FRAMES;
    if (newTicks < this->ticksRemaining[ownerPlayerId]) newTicks = 0xFFFFFFFF;
    this->ticksRemaining[ownerPlayerId] = newTicks;
}

void Mgr::OnRaceFrame() {
    Raceinfo* raceInfo = Raceinfo::sInstance;
    if (raceInfo == nullptr) return;
    RaceinfoPlayer** players = raceInfo->players;
    if (IsInvalidPTR(players)) return;
    const bool isOffline = IsOfflineCountdownRace();
    const bool isOnline = IsOnlineCountdownRace();
    if (!isOffline && !isOnline) return;
    const u16 participantMask = BuildParticipantMask(raceInfo);
    if (participantMask == 0) return;
    const u8* playerIdInEachPosition = GetValidPositionOrder(raceInfo);
    const bool isRacing = raceInfo->stage >= RACESTAGE_RACE
                       && raceInfo->stage < RACESTAGE_FINISHED;
    bool allOfflineTimersExpired = isOffline;
    bool allOnlineTimersExpired = isOnline;
    u8 activeRacersRemaining = 0;
    u8 rankMs[12];
    for (u8 i = 0; i < 12; ++i) rankMs[i] = 12;
    if (playerIdInEachPosition != nullptr) {
        for (u8 pos = 0; pos < 12; ++pos) {
            const u8 pid = playerIdInEachPosition[pos];
            if (pid >= 12) continue;
            RaceinfoPlayer* p = GetValidRaceinfoPlayer(players, pid);
            if (p == 0) continue;
            rankMs[pid] = static_cast<u8>(pos + 1);
        }
    }

    for (u8 playerId = 0; playerId < 12; ++playerId) {
        if ((participantMask & static_cast<u16>(1u << playerId)) == 0) continue;

        if (isRacing && this->ticksRemaining[playerId] > 0) {
            this->ticksRemaining[playerId]--;
        }
        if (isRacing && this->ticksRemaining[playerId] == 0 && this->overtimeFrames[playerId] < 0xFFFFFFFFu) {
            this->overtimeFrames[playerId]++;
        }

        if (isOffline) {
            if (this->ticksRemaining[playerId] > 0) {
                allOfflineTimersExpired = false;
                ++activeRacersRemaining;
            } else {
                if (!this->offlineExpired[playerId]) {
                    this->offlineExpired[playerId] = true;
                    if (!IsOfflineLocalPlayer(playerId) && this->offlineElimCount < 12) {
                        this->offlineElimOrder[this->offlineElimCount++] = playerId;
                    }
                }
                if (!IsOfflineLocalPlayer(playerId)) {
                    this->FreezeOfflineExpiredCpu(playerId);
                }
            }
            continue;
        }
        // ONLINE branch.
        //  - ticks > 0 → racing.
        //  - The frame ticks hit 0, mark expired (gates local-spectate and
        //    the all-expired finalize trigger). NO kart freeze, NO position
        //    snapshot — the kart stays in the race and progresses naturally,
        //    so final placements track normal race progress at finalize time.
        if (this->ticksRemaining[playerId] > 0) {
            allOnlineTimersExpired = false;
            ++activeRacersRemaining;
        } else if (!this->onlineExpired[playerId]) {
            this->onlineExpired[playerId] = true;
            if (this->onlineElimCount < 12) {
                this->onlineElimOrder[this->onlineElimCount++] = playerId;
            }
        }
    }

    for (u8 playerId = 0; playerId < 12; ++playerId) {
        const u32 currentHits = ReadPlayerItemsHitCount(playerId);
        if (currentHits == 0xFFFFFFFFu) continue;
        const u32 previousHits = sLastTalliedItemsHitCount[playerId];
        sLastTalliedItemsHitCount[playerId] = currentHits;
        if (!isRacing) continue;
        if (this->ticksRemaining[playerId] == 0) continue;
        if (previousHits == 0xFFFFFFFFu) continue;  // baseline first observed value
        if (currentHits <= previousHits) continue;
        this->OnSoloHit(playerId, static_cast<u16>(0xD000u | (raceInfo->raceFrames & 0x0FFFu)));
    }

    bool anyLocalTimedOut = false;
    u8 localIds[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    const u8 localCount = BuildLocalPlayerList(localIds, 4);
    for (u8 i = 0; i < localCount; ++i) {
        if (this->ticksRemaining[localIds[i]] == 0) {
            anyLocalTimedOut = true;
        }
    }
    this->localSpectating = anyLocalTimedOut && activeRacersRemaining > 0;

    if (isOffline && allOfflineTimersExpired) {
        bool shouldFinalize = false;
        for (u8 playerId = 0; playerId < 12; ++playerId) {
            if ((participantMask & static_cast<u16>(1u << playerId)) != 0 && !this->raceEnded[playerId]) {
                shouldFinalize = true;
                break;
            }
        }
        if (shouldFinalize) {
            this->FinalizeOfflinePlacementsAndEnd();
            return;
        }
    } else if (isOnline && allOnlineTimersExpired) {
        bool shouldFinalize = false;
        for (u8 playerId = 0; playerId < 12; ++playerId) {
            if ((participantMask & static_cast<u16>(1u << playerId)) != 0 && !this->raceEnded[playerId]) {
                shouldFinalize = true;
                break;
            }
        }
        if (shouldFinalize) {
            this->FinalizeOnlinePlacementsAndEnd();
            return;
        }
    }

    if (raceInfo->timerMgr != nullptr) {
        const u8 localId = GetLocalPlayerId();
        if (localId < 12) {
            const u32 ticks = this->ticksRemaining[localId];
            RaceTimerMgr* tm = raceInfo->timerMgr;
            u16 minutes = 0;
            u8 seconds = 0;
            u16 millis = 0;
            if (ticks > 0) {
                minutes = static_cast<u16>(ticks / (FRAMES_PER_SECOND * 60));
                seconds = static_cast<u8>((ticks / FRAMES_PER_SECOND) % 60);
                millis = static_cast<u16>(((ticks % FRAMES_PER_SECOND) * 1000u) / FRAMES_PER_SECOND);
                tm->isTimerReversed = true;
            } else {
                const u32 overtime = this->overtimeFrames[localId];
                const u32 maxShownFrames = ((99u * 60u + 59u) * FRAMES_PER_SECOND) + (FRAMES_PER_SECOND - 1);
                const u32 shownFrames = (overtime > maxShownFrames) ? maxShownFrames : overtime;
                minutes = static_cast<u16>(shownFrames / (FRAMES_PER_SECOND * 60));
                seconds = static_cast<u8>((shownFrames / FRAMES_PER_SECOND) % 60);
                millis = static_cast<u16>(((shownFrames % FRAMES_PER_SECOND) * 1000u) / FRAMES_PER_SECOND);
                tm->isTimerReversed = isOnline ? true : false;
            }

            tm->timers[2].minutes = minutes;
            tm->timers[2].seconds = seconds;
            tm->timers[2].milliseconds = millis;
            tm->timers[2].isActive = true;
            tm->timers[1].minutes = tm->timers[2].minutes;
            tm->timers[1].seconds = tm->timers[2].seconds;
            tm->timers[1].milliseconds = tm->timers[2].milliseconds;
            tm->timers[1].isActive = true;
            tm->raceDurationMs = 0x7fffffffu;
            tm->hasRaceTimeRanOut = false;
            if (isOnline && !allOnlineTimersExpired) {
                const u32 kOnlineForceEndMs = 300000u;
                const u32 t0Ms = TimerToMs(tm->timers[0]);
                if (t0Ms >= kOnlineForceEndMs) {
                    SetTimerFromMs(tm->timers[0], kOnlineForceEndMs - 1u);
                }
            }
        }
    }
    UpdateCountdownSpectatorView();
}

static void CreateMgrIfNeeded() {
    System* sys = System::sInstance;
    if (sys == nullptr || sys->countdownMgr != nullptr) return;
    if (!IsCountdownActive(sys)) return;
    sys->countdownMgr = new (sys->heap) Mgr;
}
RaceLoadHook CreateMgr(CreateMgrIfNeeded);

static void ResetForRace() {
    System* sys = System::sInstance;
    if (sys == nullptr || sys->countdownMgr == nullptr) return;
    sys->countdownMgr->OnRaceLoad();
}
RaceLoadHook ResetMgr(ResetForRace);

static void TickPerFrame() {
    System* sys = System::sInstance;
    if (!IsCountdownActive(System::sInstance)) return;
    if (sys == nullptr || sys->countdownMgr == nullptr) return;
    sys->countdownMgr->OnRaceFrame();
}
RaceFrameHook FrameTick(TickPerFrame);

static bool CountdownTryEndRaceHook(void* raceMode, u32 finishedCount, u32 playerCount) {
    if (IsCountdownActive(System::sInstance)) {
        System* sys = System::sInstance;
        if (sys == nullptr || sys->countdownMgr == nullptr) return false;
        Raceinfo* raceInfo = Raceinfo::sInstance;
        RaceinfoPlayer** players = (raceInfo != nullptr) ? raceInfo->players : 0;
        if (raceInfo == nullptr || raceInfo->stage < RACESTAGE_RACE) return false;
        if (IsInvalidPTR(players)) return false;
        const u16 participantMask = BuildParticipantMask(raceInfo);
        if (participantMask == 0) return false;
        for (u8 playerId = 0; playerId < 12; ++playerId) {
            if ((participantMask & static_cast<u16>(1u << playerId)) == 0) continue;
            if (sys->countdownMgr->GetTicksRemaining(playerId) > 0) return false;
        }
        const RKNet::Controller* controller = RKNet::Controller::sInstance;
        const bool isOffline = (controller == nullptr) || (controller->roomType == RKNet::ROOMTYPE_NONE);
        if (!isOffline) {
            return finishedCount >= playerCount;
        }
    }
    if (finishedCount < playerCount) {
        typedef void (*RaceModeEndRaceFunc)(void* self);
        if (raceMode == nullptr) return false;
        if ((reinterpret_cast<u32>(raceMode) & 0x3u) != 0) return false;
        void** vtable = *(void***)raceMode;
        if (vtable == nullptr) return false;
        if ((reinterpret_cast<u32>(vtable) & 0x3u) != 0) return false;
        RaceModeEndRaceFunc endRace = reinterpret_cast<RaceModeEndRaceFunc>(vtable[3]);  // +0xC
        if (endRace == nullptr) return false;
        endRace(raceMode);
    }
    return finishedCount >= playerCount;
}
kmBranch(0x80533c34, CountdownTryEndRaceHook);

static bool IsDirectCollisionScoringItem(ItemObjId id);

static void OnItemPlayerCollision(Item::Obj* obj, Kart::Player& player, bool isRemote) {
    obj->OnPlayerCollision(player, isRemote);
    if (!IsCountdownActive(System::sInstance)) return;
    if (obj == nullptr) return;
    const u8 ownerId = ResolveItemOwnerId(*obj);
    const u8 victimId = player.GetPlayerIdx();
    Raceinfo* ri = Raceinfo::sInstance;
    const u32 frame = (ri != nullptr) ? ri->raceFrames : 0;
    if (ownerId < 12 && victimId < 12 && ownerId != victimId) {
        sLastCollisionOwnerByVictim[victimId] = ownerId;
        sLastCollisionFrameByVictim[victimId] = frame;
    }
}
kmCall(0x8079549c, OnItemPlayerCollision);
kmCall(0x80799e14, OnItemPlayerCollision);

static bool IsDirectCollisionScoringItem(ItemObjId id) {
    switch (id) {
        case OBJ_GREEN_SHELL:
        case OBJ_RED_SHELL:
        case OBJ_BANANA:
        case OBJ_FAKE_ITEM_BOX:
            return true;
        default:
            return false;
    }
}

static void CountdownKillFromPlayerCollisionHook(Item::Obj* obj, bool sendBreakEVENT, u8 playerIdOfCollision) {
    obj->KillFromPlayerCollision(sendBreakEVENT, playerIdOfCollision);
    if (!IsCountdownActive(System::sInstance)) return;
    if (obj == nullptr || !IsDirectCollisionScoringItem(obj->itemObjId)) return;

    System* sys = System::sInstance;
    if (sys == nullptr || sys->countdownMgr == nullptr) return;
    const u8 ownerId = ResolveItemOwnerId(*obj);
    const u8 victimId = playerIdOfCollision;
    if (ownerId >= 12 || victimId >= 12 || ownerId == victimId) return;

    Raceinfo* ri = Raceinfo::sInstance;
    const u32 frame = (ri != nullptr) ? ri->raceFrames : 0;
    sLastCollisionOwnerByVictim[victimId] = ownerId;
    sLastCollisionFrameByVictim[victimId] = frame;
}
kmCall(0x8079fc2c, CountdownKillFromPlayerCollisionHook);
kmCall(0x807a3838, CountdownKillFromPlayerCollisionHook);
kmCall(0x807ab2a4, CountdownKillFromPlayerCollisionHook);
kmCall(0x807ab350, CountdownKillFromPlayerCollisionHook);
kmCall(0x807b33e8, CountdownKillFromPlayerCollisionHook);

static void CtrlRaceLapOnUpdate(CtrlRaceLap* self) {
    self->UpdatePausePosition();

    Raceinfo* ri = Raceinfo::sInstance;
    if (ri == nullptr || ri->players == nullptr) return;
    const u8 id = self->GetPlayerId();
    RaceinfoPlayer* p = ri->players[id];
    if (p == nullptr) return;

    if (self->maxLap != p->maxLap) {
        self->maxLap = p->maxLap;
        AnimationGroup& g0 = self->animator.GetAnimationGroupById(0);
        g0.PlayAnimationAtFrameAndDisable(0, (float)p->maxLap);
    }

    if (!IsCountdownActive(System::sInstance)) return;
    System* sys = System::sInstance;
    if (sys == nullptr || sys->countdownMgr == nullptr) return;
    u8 score = sys->countdownMgr->GetScore(id);
    if (score > Mgr::MAX_SCORE) score = Mgr::MAX_SCORE;
    AnimationGroup& g1 = self->animator.GetAnimationGroupById(1);
    g1.PlayAnimationAtFrameAndDisable(0, (float)score);
}
kmBranch(0x807ef810, CtrlRaceLapOnUpdate);

extern "C" bool KartDamage_SetDamage(Kart::Damage* self, u32 newDamage, u32 unused1,
                                     bool affectsMegas, u32* appliedDamage,
                                     u32 playerObjIdx, u32 unused2);

static bool CountdownSetDamageHook(Kart::Damage* self, u32 newDamage, u32 unused1,
                                    bool affectsMegas, u32* appliedDamage,
                                    u32 playerObjIdx, u32 unused2) {
    if (self != nullptr) {
        const u8 victimId = self->GetPlayerIdx();
        if (IsOfflineOutOfTimeCpu(victimId)) {
            // Keep expired offline CPUs targetable without entering endless hit states.
            if (IsCountdownActive(System::sInstance) && playerObjIdx < 12 && playerObjIdx != victimId) {
                bool isScoring = false;
                switch (newDamage) {
                    case SPINOUT_BANANA:
                    case KNOCKBACK_SHELL_FIB:
                    case KNOCKBACK_STAR:
                    case KNOCKBACK_BULLET:
                    case LAUNCH_EXPLOSION:
                    case SPINOUT_SHOCK:
                    case POW:
                    case SQUISH_MEGA:
                        isScoring = true;
                        break;
                    default:
                        break;
                }
                if (isScoring) {
                    System* sys = System::sInstance;
                    if (sys != nullptr && sys->countdownMgr != nullptr) {
                        Raceinfo* ri = Raceinfo::sInstance;
                        const u32 frame = (ri != nullptr) ? ri->raceFrames : 0;
                        const u16 instanceIdx = static_cast<u16>(((newDamage & 0x1F) << 8) | (frame & 0xFF));
                        sys->countdownMgr->OnItemHit(static_cast<u8>(playerObjIdx), victimId, instanceIdx);
                    }
                }
            }
            return false;
        }
    }

    const bool result = KartDamage_SetDamage(self, newDamage, unused1, affectsMegas,
                                             appliedDamage, playerObjIdx, unused2);
    return result;
}
kmCall(0x80569f54, CountdownSetDamageHook);
kmCall(0x80569fdc, CountdownSetDamageHook);
kmCall(0x8058d6b8, CountdownSetDamageHook);
kmWritePointer(0x808b5014, CountdownSetDamageHook);

extern "C" void KartObjectProxy_incrementHitOtherCount(void* self);
extern "C" u8 KartObjectProxy_GetPlayerIdx(const void* self);

static void CountdownAOEHitHook(void* aggressorProxy) {
    KartObjectProxy_incrementHitOtherCount(aggressorProxy);
    if (!IsCountdownActive(System::sInstance)) return;
}

kmCall(0x807a9488, CountdownAOEHitHook);  // GessoManager_calc — Blooper landed
kmCall(0x807b2454, CountdownAOEHitHook);  // PowManager_calc — POW landed
kmCall(0x807b7d5c, CountdownAOEHitHook);  // activateLightning — Lightning landed

extern "C" void KartCollide_bumpPlayer(double p1, void* self, void* bumper,
                                        void* victim, void* v1, void* v2);

static const u32 kMushroomBoostBit = 0x4000000u;
static u32 ReadKartBitfield0(void* proxy) {
    if (proxy == nullptr) return 0;
    void* mp = *(void**)proxy;
    if (mp == nullptr) return 0;
    void* status = *(void**)((char*)mp + 0x4);
    if (status == nullptr) return 0;
    return *(u32*)((char*)status + 0x4);
}

static void CountdownBumpPlayerHook(double p1, void* self, void* bumper,
                                     void* victim, void* v1, void* v2) {
    KartCollide_bumpPlayer(p1, self, bumper, victim, v1, v2);
    if (!IsCountdownActive(System::sInstance)) return;
    if (bumper == nullptr || victim == nullptr) return;

    if ((ReadKartBitfield0(bumper) & kMushroomBoostBit) == 0) return;
    if ((ReadKartBitfield0(victim) & kMushroomBoostBit) != 0) return;  // both boosted: no score

    System* sys = System::sInstance;
    if (sys == nullptr || sys->countdownMgr == nullptr) return;
    const u8 ownerId = KartObjectProxy_GetPlayerIdx(bumper);
    const u8 victimId = KartObjectProxy_GetPlayerIdx(victim);
    if (ownerId >= 12 || victimId >= 12 || ownerId == victimId) return;

    Raceinfo* ri = Raceinfo::sInstance;
    const u32 frame = (ri != nullptr) ? ri->raceFrames : 0;
    if (sLastCollisionOwnerByVictim[victimId] == ownerId
        && frame - sLastCollisionFrameByVictim[victimId] < Mgr::HIT_COOLDOWN_FRAMES) {
        return;
    }

    // If vanilla tallies already captured this bump, let the tally-based scorer handle it.
    const u32 hitsNow = ReadPlayerItemsHitCount(ownerId);
    const u32 hitsPrev = sLastTalliedItemsHitCount[ownerId];
    if (hitsNow != 0xFFFFFFFFu && hitsPrev != 0xFFFFFFFFu && hitsNow > hitsPrev) return;

    sLastCollisionOwnerByVictim[victimId] = ownerId;
    sLastCollisionFrameByVictim[victimId] = frame;
    sys->countdownMgr->OnSoloHit(ownerId, static_cast<u16>(0xC000 | (frame & 0x0FFFu)));
}
kmCall(0x80570814, CountdownBumpPlayerHook);  // calcPlayerCollision: this bumps other
kmCall(0x80570850, CountdownBumpPlayerHook);  // calcPlayerCollision: other bumps this

extern "C" void KartSub_Update(Kart::Sub* self);
static void CountdownCalcSubHook(Kart::Player* player) {
    Kart::Sub* self = (player != nullptr) ? player->kartSub : 0;
    if (self != 0 && IsCountdownActive(System::sInstance)) {
        const RKNet::Controller* controller = RKNet::Controller::sInstance;
        const bool isOffline = (controller == nullptr) || (controller->roomType == RKNet::ROOMTYPE_NONE);
        if (isOffline) {
            System* sys = System::sInstance;
            if (sys != nullptr && sys->countdownMgr != nullptr) {
                const u8 playerId = self->GetPlayerIdx();
                if (playerId < 12 && !IsOfflineLocalPlayer(playerId)) {
                    const bool outOfTime = (sys->countdownMgr->GetTicksRemaining(playerId) == 0)
                                        || sys->countdownMgr->IsOfflineExpired(playerId);
                    if (outOfTime) {
                        sys->countdownMgr->FreezeOfflineExpiredCpu(playerId);
                        return;  // never run KartSub::Update for out-of-time CPUs
                    }
                }
            }
        }
    }
    if (self != 0) KartSub_Update(self);
}
kmBranch(0x8058eeb4, CountdownCalcSubHook);

extern "C" void RaceManagerPlayer_endRace(void* self, const void* finishTime,
                                          u32 hasNoCameras, u32 r6);

static void CountdownSuppressNaturalLapFinish(void* self, const void* finishTime,
                                              u32 hasNoCameras, u32 r6) {
    if (IsCountdownActive(System::sInstance)) return;
    RaceManagerPlayer_endRace(self, finishTime, hasNoCameras, r6);
}
kmCall(0x80534c20, CountdownSuppressNaturalLapFinish);  // endLap → endRace (natural lap finish)
kmCall(0x80534664, CountdownSuppressNaturalLapFinish);  // endLocalRace → endRace
kmCall(0x80534d34, CountdownSuppressNaturalLapFinish);  // FUN_80534ccc → endRace
kmCall(0x80534d48, CountdownSuppressNaturalLapFinish);  // FUN_80534ccc → endRace

extern "C" void RaceManagerPlayer_endLocalRace(void* player, u32 a, u32 b);

static void CountdownSuppressOnlineVsForceEnd(void* player, u32 a, u32 b) {
    if (IsCountdownActive(System::sInstance)) return;
    RaceManagerPlayer_endLocalRace(player, a, b);
}
kmCall(0x8053f434, CountdownSuppressOnlineVsForceEnd);


extern "C" void KartMove_respawn(Kart::Movement* self);
extern "C" void KartMove_oobRecover(Kart::Movement* self);

static void MarkExpiredCpuStopped(Kart::Movement* self) {
    if (self == nullptr || self->pointers == nullptr || self->pointers->kartStatus == nullptr) return;
    Kart::Status* status = self->pointers->kartStatus;
    status->bitfield1 &= ~0x2u;         // TRIGGER_RESPAWN
    status->bitfield0 &= ~0x10u;        // BEFORE_RESPAWN
    status->bitfield0 &= ~(0x2u | 0x4u | 0x8u | 0x2000u | 0x1000000u);
    status->bitfield0 |= 0x1u;          // accelerate
    status->stickX = 0.0f;
    status->stickY = 0.0f;
    status->bitfield2 &= ~0x0008E000u;  // clear vanished + post-respawn flags
    status->bitfield2 &= ~0x08000000u;  // clear in-bullet
    status->bitfield2 &= ~0x00040000u;  // keep collisions enabled
    status->bitfield4 &= ~0x00001000u;  // keep collisions enabled
    status->bitfield4 &= ~0x00180000u;  // clear OOB transition flags
}

}  // namespace Countdown
}  // namespace Pulsar
