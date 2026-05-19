#include <kamek.hpp>
#include <MarioKartWii/Item/ItemManager.hpp>
#include <MarioKartWii/Item/Obj/ItemObjHolder.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/Item/Obj/ItemObj.hpp>
#include <MarioKartWii/Kart/KartPlayer.hpp>
#include <PulsarSystem.hpp>
#include <SlotExpansion/CupsConfig.hpp>
#include <Network/PacketExpansion.hpp>

namespace Pulsar {
namespace ItemRain {

static const float SPAWN_HEIGHT = 3000.0f;
static const float SPAWN_RADIUS = 10000.0f;
static int MAX_ITEM_LIFETIME = 1200;
static int DESPAWN_CHECK_INTERVAL = 2;
static int FindLocalPlayerId();
static bool IsOnline();

static u8 sLastReceivedSeq = 0xFF;
static s32 sRaceInfoFrameCounter = 0;
static u32 sRngCounter = 0;
static u8 sSpawnSeqCounter = 0;

// Per-player isolation-aware spawn system
static float sPlayerSpawnAccum[12] = {};
static u8 sSpawnScanStart = 0;
static const float NEIGHBOR_RADIUS_SQ = 20000.0f * 20000.0f; // Players within 20000 units share visible items
static const float TARGET_VISIBLE_RATE = 8.0f;        // Items/sec each player should see (Item Rain)
static const float TARGET_VISIBLE_RATE_MAYHEM = 2.0f;  // Items/sec each player should see (Mayhem)
static const float MAX_SPAWN_RATE = 15.0f;             // Cap per-player spawn rate

static u8 CountNearbyPlayers(u8 idx, u8 playerCount) {
    Vec3 pos = Item::Manager::sInstance->players[idx].GetPosition();
    u8 count = 0;
    for (u8 j = 0; j < playerCount && j < 12; j++) {
        Vec3 other = Item::Manager::sInstance->players[j].GetPosition();
        float dx = pos.x - other.x;
        float dy = pos.y - other.y;
        float dz = pos.z - other.z;
        if (dx * dx + dy * dy + dz * dz < NEIGHBOR_RADIUS_SQ) {
            count++;
        }
    }
    return count < 1 ? 1 : count;
}

#pragma pack(push, 4)
struct ItemRainBroadcast {
    Vec3 pos;
    u16 eventField;
    bool active;
    u8 seq;
    u8 itemId;
    u8 targetPlayer;
    u8 hostPlayerId;
    u8 ticksRemaining;
};
#pragma pack(pop)

// Currently broadcasting item (retransmit for reliability)
static ItemRainBroadcast sPendingBroadcast;

static inline void ResetPendingBroadcast() {
    sPendingBroadcast.pos.x = 0.0f;
    sPendingBroadcast.pos.y = 0.0f;
    sPendingBroadcast.pos.z = 0.0f;
    sPendingBroadcast.eventField = 0xFFFF;
    sPendingBroadcast.active = false;
    sPendingBroadcast.seq = 0xFF;
    sPendingBroadcast.itemId = 0xFF;
    sPendingBroadcast.targetPlayer = 0;
    sPendingBroadcast.hostPlayerId = 0;
    sPendingBroadcast.ticksRemaining = 0;
}


static u32 GetRandom() {
    ++sRngCounter;
    u32 value = (sRaceInfoFrameCounter * 1103515245 + sRngCounter * 12345) ^ (sRngCounter << 8);
    return value * 1664525 + 1013904223;
}

static float GetRandomRange(float min, float max) {
    u32 r = GetRandom() & 0xFFFF;
    union { u32 i; float f; } conv;
    conv.i = 0x3F800000u | (r << 7);
    float frac = conv.f - 1.0f;
    return min + frac * (max - min);
}

static ItemObjId GetRandomItem() {
    struct ItemWeight {
        ItemObjId id;
        u32 weight;
    };

    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    static const ItemWeight weightsVS[] = {
        {OBJ_MUSHROOM, 200},
        {OBJ_GREEN_SHELL, 170},
        {OBJ_BANANA, 170},
        {OBJ_RED_SHELL, 170},
        {OBJ_FAKE_ITEM_BOX, 170},
        {OBJ_BOBOMB, 30},
        {OBJ_STAR, 26},
        {OBJ_GOLDEN_MUSHROOM, 26},
        {OBJ_MEGA_MUSHROOM, 26},
        {OBJ_BLOOPER, 5},
        {OBJ_BULLET_BILL, 4},
        {OBJ_LIGHTNING, 3},
    };
    static const ItemWeight weightsBattle[] = {
        {OBJ_MUSHROOM, 200},
        {OBJ_GREEN_SHELL, 170},
        {OBJ_BANANA, 170},
        {OBJ_RED_SHELL, 170},
        {OBJ_FAKE_ITEM_BOX, 170},
        {OBJ_BOBOMB, 30},
        {OBJ_STAR, 26},
        {OBJ_GOLDEN_MUSHROOM, 26},
        {OBJ_MEGA_MUSHROOM, 26},
        {OBJ_BLOOPER, 8},
        {OBJ_LIGHTNING, 4},
    };
    static const ItemWeight weightsMayhem[] = {
        {OBJ_GREEN_SHELL, 150},
        {OBJ_RED_SHELL, 150},
        {OBJ_BANANA, 700},
    };

    const ItemWeight* weights = weightsVS;
    u32 count = sizeof(weightsVS) / sizeof(weightsVS[0]);
    if (mode == MODE_BATTLE || mode == MODE_PRIVATE_BATTLE) {
        weights = weightsBattle;
        count = sizeof(weightsBattle) / sizeof(weightsBattle[0]);
    } else if ((RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_HOST ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_FROOM_NONHOST ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_NONE ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_VS_REGIONAL ||
        RKNet::Controller::sInstance->roomType == RKNet::ROOMTYPE_JOINING_REGIONAL) && 
        System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM)) {
        weights = weightsMayhem;
        count = sizeof(weightsMayhem) / sizeof(weightsMayhem[0]);
    }

    u32 totalWeight = 0;
    for (u32 i = 0; i < count; i++) totalWeight += weights[i].weight;
    if (totalWeight == 0) return OBJ_MUSHROOM;

    u32 roll = GetRandom() % totalWeight;
    u32 cumulative = 0;
    for (u32 i = 0; i < count; i++) {
        cumulative += weights[i].weight;
        if (roll < cumulative) return weights[i].id;
    }
    return OBJ_MUSHROOM;
}

void DespawnItems(bool checkDistance = false) {
    if (!Item::Manager::sInstance) return;
    const bool online = IsOnline();
    u8 playerCount = 0;
    Vec3 playerPositions[12];
    if (checkDistance && !online) {
        playerCount = Pulsar::System::sInstance ? Pulsar::System::sInstance->nonTTGhostPlayersCount : 0;
        for (int i = 0; i < playerCount && i < 12; i++) {
            playerPositions[i] = Item::Manager::sInstance->players[i].GetPosition();
        }
    }
    for (int i = 0; i < 0xF; i++) {
        Item::ObjHolder& holder = Item::Manager::sInstance->itemObjHolders[i];
        if (holder.itemObjId == OBJ_NONE || holder.bodyCount == 0 || holder.itemObjId == OBJ_THUNDER_CLOUD) continue;
        for (u32 j = 0; j < holder.capacity; j++) {
            Item::Obj* obj = holder.itemObj[j];
            if (!obj || (obj->bitfield74 & 0x1)) continue;
            bool shouldDespawn = obj->duration > MAX_ITEM_LIFETIME;
            if (checkDistance && !online && obj->duration >= 300) {
                bool farFromAll = true;
                for (int k = 0; k < playerCount && k < 12 && farFromAll; k++) {
                    Vec3 diff;
                    diff.x = obj->position.x - playerPositions[k].x;
                    diff.y = obj->position.y - playerPositions[k].y;
                    diff.z = obj->position.z - playerPositions[k].z;
                    if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z < 225000000.0f) {
                        farFromAll = false;
                    }
                }
                shouldDespawn |= farFromAll;
            }
            if (shouldDespawn) {
                obj->DisappearDueToExcess(online); // send break event in online to sync removal
            }
        }
    }
}

static bool IsHost() {
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    if (!controller) return true;
    if (controller->roomType == RKNet::ROOMTYPE_NONE) return true;
    const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];
    return sub.localAid == sub.hostAid;
}

static bool IsOnline() {
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    if (!controller) return false;
    return controller->roomType != RKNet::ROOMTYPE_NONE;
}

static void BroadcastItemRain(const ItemRainBroadcast& data) {
    if (!IsOnline() || !data.active) return;
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];

    for (int aid = 0; aid < 12; ++aid) {
        if (((1 << aid) & sub.availableAids) == 0 || aid == sub.localAid) continue;
        RKNet::PacketHolder<Network::PulRH1>* holder = controller->GetSendPacketHolder<Network::PulRH1>(aid);
        Network::PulRH1* dest = holder->packet;
        dest->itemRainSeq = data.seq;
        dest->itemRainItemId = data.itemId;
        dest->itemRainTargetPlayer = data.targetPlayer;
        dest->itemRainHostPlayerId = data.hostPlayerId;
        memcpy(&dest->itemRainPosX, &data.pos.x, sizeof(u32));
        memcpy(&dest->itemRainPosY, &data.pos.y, sizeof(u32));
        memcpy(&dest->itemRainPosZ, &data.pos.z, sizeof(u32));
        dest->itemRainEventField = data.eventField;
    }
}

static inline u32 GetEntryTimestamp(const RKNet::EVENTEntry& e) {
    u32 ts;
    memcpy(&ts, &e, sizeof(u32));
    return ts;
}

static inline void FreeEventEntry(RKNet::EVENTEntry& e) {
    memset(&e, 0, sizeof(RKNet::EVENTEntry));
    e.state = RKNet::EVENTENTRYSTATE_FREE;
    e.itemObjId = 0x10;
}

static void ForceFlushExpiredEvents() {
    RKNet::EVENTHandler* handler = RKNet::EVENTHandler::sInstance;
    if (!handler || !handler->isPrepared) return;

    bool freedAny = false;
    for (u32 i = 0; i < 24; i++) {
        RKNet::EVENTEntry& e = handler->toSendEntries[i];
        if (e.state == RKNet::EVENTENTRYSTATE_EXPIRED) {
            FreeEventEntry(e);
            freedAny = true;
        }
    }

    if (freedAny) {
        u32 usedData = 0;
        for (u32 i = 0; i < 24; i++) {
            if (handler->toSendEntries[i].state == RKNet::EVENTENTRYSTATE_FULL) {
                usedData += handler->toSendEntries[i].dataLength;
            }
        }
        handler->freeDataInSendBuffer = 0xe0 - usedData;
    }
}

static void CleanupPickedUpItems() {
    if (!Item::Manager::sInstance) return;

    for (int i = 0; i < 0xF; i++) {
        Item::ObjHolder& holder = Item::Manager::sInstance->itemObjHolders[i];
        if (holder.itemObjId == OBJ_NONE || holder.bodyCount == 0) continue;
        if (holder.itemObjId == OBJ_BOBOMB) continue;
        for (u32 j = 0; j < holder.capacity; j++) {
            Item::Obj* obj = holder.itemObj[j];
            if (!obj) continue;

            if (obj->bitfield74 & 0x1) {
                obj->DisappearDueToExcess(true); // force network break event as backup sync
            }
        }
    }
}

// Flush mushroom events on-demand to reclaim slots/data when we are out of room.
static bool FlushMushroomEvents() {
    RKNet::EVENTHandler* handler = RKNet::EVENTHandler::sInstance;
    if (!handler || !handler->isPrepared) return false;

    bool freedAny = false;
    for (u32 i = 0; i < 24; i++) {
        RKNet::EVENTEntry& e = handler->toSendEntries[i];
        if (e.state == RKNet::EVENTENTRYSTATE_FULL &&
            (e.itemObjId == OBJ_MUSHROOM || e.itemObjId == OBJ_GOLDEN_MUSHROOM)) {
            FreeEventEntry(e);
            freedAny = true;
        }
    }

    if (freedAny) {
        u32 usedData = 0;
        for (u32 i = 0; i < 24; i++) {
            if (handler->toSendEntries[i].state == RKNet::EVENTENTRYSTATE_FULL) {
                usedData += handler->toSendEntries[i].dataLength;
            }
        }
        handler->freeDataInSendBuffer = 0xe0 - usedData;
    }
    return freedAny;
}

static void SpawnItemAtPosition(ItemObjId selectedItem, const Vec3& spawnPos, u8 hostPlayerId, u16* outEventField = nullptr, u16 overrideEventField = 0xFFFF) {
    if (!Item::Manager::sInstance) return;
    if (selectedItem >= 0xF) return;

    Vec3 dummyDirection;
    dummyDirection.x = 0.0f;
    dummyDirection.y = 0.0f;
    dummyDirection.z = 0.0f;

    u8 creatorPlayerId = hostPlayerId;
    if (!IsHost()) {
        int localId = FindLocalPlayerId();
        if (localId >= 0) creatorPlayerId = static_cast<u8>(localId);
    }

    Item::ObjHolder& holder = Item::Manager::sInstance->itemObjHolders[selectedItem];

        if (holder.bodyCount >= holder.capacity2 && holder.capacity > 0) {
        Item::Obj* oldest = nullptr;
        u32 maxDuration = 0;
        for (u32 j = 0; j < holder.capacity; j++) {
            Item::Obj* candidate = holder.itemObj[j];
            if (!candidate || (candidate->bitfield74 & 0x1)) continue;
            if (candidate->duration >= maxDuration) {
                maxDuration = candidate->duration;
                oldest = candidate;
            }
        }
        if (oldest) {
                oldest->DisappearDueToExcess(true); // notify others when replacing oldest
        }
    }

    Vec3 pos = spawnPos;
    bool spawned = false;
    for (int attempt = 0; attempt < 3 && !spawned; attempt++) {
        u32 countBefore = holder.spawnedCount;
        Item::Manager::sInstance->CreateItemDirect(selectedItem, &pos, &dummyDirection, creatorPlayerId);

        if (holder.spawnedCount > countBefore) {
            Item::Obj* obj = holder.itemObj[holder.spawnedCount - 1];
            if (obj) {
                if (overrideEventField != 0xFFFF) {
                    obj->eventBitfield = overrideEventField;
                }
                if (outEventField) *outEventField = obj->eventBitfield;

                obj->playerUsedItemId = hostPlayerId;
                obj->playerCollisionId = hostPlayerId;

                if (IsOnline()) {
                    obj->bitfield7c |= 0x20;
                }
                obj->bitfield7c |= 0x12;
            }
            spawned = true;
        }
    }
}

static int FindLocalPlayerId() {
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    u8 playerCount = Pulsar::System::sInstance->nonTTGhostPlayersCount;
    for (int i = 0; i < playerCount; i++) {
        if (scenario.players[i].playerType == PLAYER_REAL_LOCAL) {
            return i;
        }
    }
    return -1;
}

static void HostSpawnForPlayer(u32 raceFrames, u8 hostPlayerId, u8 targetPlayerIdx) {
    Item::Player& player = Item::Manager::sInstance->players[targetPlayerIdx];
    Vec3 playerPos = player.GetPosition();
    Vec3 forwardDir;
    if (player.kartPlayer) {
        forwardDir = player.kartPlayer->GetMovement().dir;
    } else {
        forwardDir.x = 0.0f;
        forwardDir.y = 0.0f;
        forwardDir.z = 1.0f;
    }
    Vec3 rightDir;
    rightDir.x = forwardDir.z;
    rightDir.y = 0.0f;
    rightDir.z = -forwardDir.x;

    float forward = GetRandomRange(1000.0f, 12000.0f);
    float side = GetRandomRange(-SPAWN_RADIUS, SPAWN_RADIUS);

    Vec3 spawnPos;
    spawnPos.x = playerPos.x + forwardDir.x * forward + rightDir.x * side;
    spawnPos.y = playerPos.y + SPAWN_HEIGHT;
    spawnPos.z = playerPos.z + forwardDir.z * forward + rightDir.z * side;

    ItemObjId selectedItem = GetRandomItem();
    if (selectedItem >= 0xF) return;
    u16 eventField = 0xFFFF;

    // Host spawns directly to get the real eventBitfield for sync
    SpawnItemAtPosition(selectedItem, spawnPos, hostPlayerId, &eventField);

    // Broadcast to clients with the actual eventField so pickups stay synced
    if (IsOnline()) {
        ++sSpawnSeqCounter;
        sPendingBroadcast.active = true;
        sPendingBroadcast.seq = sSpawnSeqCounter;
        sPendingBroadcast.itemId = (u8)selectedItem;
        sPendingBroadcast.targetPlayer = targetPlayerIdx;
        sPendingBroadcast.hostPlayerId = hostPlayerId;
        sPendingBroadcast.pos = spawnPos;
        sPendingBroadcast.ticksRemaining = 3;
        sPendingBroadcast.eventField = eventField;
        BroadcastItemRain(sPendingBroadcast);
    }
}

static void ClientReceiveAndSpawn() {
    const RKNet::Controller* controller = RKNet::Controller::sInstance;
    if (!controller) return;
    const RKNet::ControllerSub& sub = controller->subs[controller->currentSub];
    const u8 hostAid = sub.hostAid;
    if (hostAid >= 12) return;

    const u32 bufferIdx = controller->lastReceivedBufferUsed[hostAid][RKNet::PACKET_RACEHEADER1];
    RKNet::SplitRACEPointers* split = controller->splitReceivedRACEPackets[bufferIdx][hostAid];
    if (!split) return;

    const RKNet::PacketHolder<Network::PulRH1>* holder = split->GetPacketHolder<Network::PulRH1>();
    if (holder->packetSize < Network::PulRH1SizeBase) return;

    const Network::PulRH1* packet = holder->packet;
    if (!packet) return;

    u8 seq = packet->itemRainSeq;
    if (seq == sLastReceivedSeq) return;
    sLastReceivedSeq = seq;

    u8 itemId = packet->itemRainItemId;
    if (itemId == 0xFF || itemId >= 0xF) return;

    Vec3 spawnPos;
    memcpy(&spawnPos.x, &packet->itemRainPosX, sizeof(float));
    memcpy(&spawnPos.y, &packet->itemRainPosY, sizeof(float));
    memcpy(&spawnPos.z, &packet->itemRainPosZ, sizeof(float));

    u8 hostPlayerId = packet->itemRainHostPlayerId;
    u16 eventField = packet->itemRainEventField;

    // Spawn directly with the host's eventField so item interactions stay synced
    SpawnItemAtPosition(static_cast<ItemObjId>(itemId), spawnPos, hostPlayerId, nullptr, eventField);
}

void SpawnItemRain() {
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    if (!Pulsar::System::sInstance->IsContext(PULSAR_MODE_ITEMRAIN) && !Pulsar::System::sInstance->IsContext(PULSAR_MODE_MAYHEM)) return;
    if (Pulsar::System::sInstance->IsContext(PULSAR_MODE_OTT)) return;
    if (RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_FROOM_HOST && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_FROOM_NONHOST &&
        RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_NONE && RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_VS_REGIONAL &&
        RKNet::Controller::sInstance->roomType != RKNet::ROOMTYPE_JOINING_REGIONAL) return;
    if (mode == MODE_TIME_TRIAL) return;
    if (!Racedata::sInstance || !Raceinfo::sInstance || !Item::Manager::sInstance) return;
    if (!Raceinfo::sInstance->IsAtLeastStage(RACESTAGE_RACE)) return;
    if (!CupsConfig::sInstance) return;

    const u32 raceFrames = Raceinfo::sInstance->raceFrames;
    sRaceInfoFrameCounter = static_cast<s32>(raceFrames);

    if (raceFrames == 0) {
        sLastReceivedSeq = 0xFF;
        sRngCounter = 0;
        sSpawnSeqCounter = 0;
        sSpawnScanStart = 0;
        for (int i = 0; i < 12; i++) sPlayerSpawnAccum[i] = 0.0f;
        ResetPendingBroadcast();
    }

    if (IsOnline()) {
        ForceFlushExpiredEvents();
        CleanupPickedUpItems();
    }

    if ((raceFrames % DESPAWN_CHECK_INTERVAL) == 0) {
        DespawnItems();
    }
    if ((raceFrames % (DESPAWN_CHECK_INTERVAL * 2)) == 0) {
        DespawnItems(true);
    }

    u8 playerCount = Pulsar::System::sInstance->nonTTGhostPlayersCount;
    if (playerCount == 0) return;

    if (IsHost()) {
        if (sPendingBroadcast.active) {
            if (sPendingBroadcast.ticksRemaining > 0) {
                BroadcastItemRain(sPendingBroadcast);
                --sPendingBroadcast.ticksRemaining;
            } else {
                sPendingBroadcast.active = false;
            }
        }

        bool isMayhem = System::sInstance->IsContext(Pulsar::PULSAR_MODE_MAYHEM);
        float targetRate = isMayhem ? TARGET_VISIBLE_RATE_MAYHEM : TARGET_VISIBLE_RATE;

        int hostPlayerId = FindLocalPlayerId();
        if (hostPlayerId < 0) return;

        // Accumulate spawn credits per player based on isolation
        for (u8 i = 0; i < playerCount && i < 12; i++) {
            u8 nearbyCount = CountNearbyPlayers(i, playerCount);
            float rate = targetRate / (float)nearbyCount;
            if (rate > MAX_SPAWN_RATE) rate = MAX_SPAWN_RATE;
            sPlayerSpawnAccum[i] += rate / 60.0f;
        }

        // Only spawn a new item if previous broadcast finished retransmitting (or offline)
        // This ensures each item gets its full 3-frame retransmit window so clients don't miss items
        bool canSpawn = !sPendingBroadcast.active || !IsOnline();

        if (canSpawn) {
            for (u8 scan = 0; scan < playerCount; scan++) {
                u8 idx = (sSpawnScanStart + scan) % playerCount;
                if (idx >= 12) continue;
                if (sPlayerSpawnAccum[idx] >= 1.0f) {
                    sPlayerSpawnAccum[idx] -= 1.0f;
                    if (sPlayerSpawnAccum[idx] > 2.0f) sPlayerSpawnAccum[idx] = 2.0f;
                    HostSpawnForPlayer(raceFrames, (u8)hostPlayerId, idx);
                    sSpawnScanStart = (idx + 1) % playerCount;
                    break;
                }
            }
        }
    } else {
        ClientReceiveAndSpawn();
    }
}
RaceFrameHook ItemRainHook(SpawnItemRain);

static void OriginalOverflowAdd(u8* handler, u32 objId, u32 action, void* data, u32 size) {
    u8 freeHead = handler[0x11];
    if (freeHead == 0xFF) return;

    u8* entries = *(u8**)(handler + 0x18);
    u8* entry   = entries + (u32)freeHead * 0x24;

    *(u32*)(entry + 0x18) = objId;
    *(u32*)(entry + 0x1c) = action;
    memcpy(entry, data, size);

    u8* links = *(u8**)(handler + 0x14);
    u32 fIdx4 = (u32)freeHead * 4;

    handler[0x11] = links[fIdx4];

    u8 usedHead = handler[0x10];
    if (usedHead == 0xFF) {
        handler[0x10] = freeHead;
    } else {
        u8 cur = usedHead;
        while ((s8)links[(u32)cur * 4] != -1) {
            cur = links[(u32)cur * 4];
        }
        links[(u32)cur * 4] = freeHead;
    }

    links[fIdx4]     = 0xFF;
    links[fIdx4 + 2] = (u8)size;
}

static const u32 RESERVED_SLOT_START = 20;
static bool HasAnyFreeEntry(RKNet::EVENTHandler* handler) {
    for (u32 i = 0; i < 24; i++) {
        if (handler->toSendEntries[i].state == RKNet::EVENTENTRYSTATE_FREE) return true;
    }
    return false;
}

static bool ItemRainHasFreeEntries(RKNet::EVENTHandler* handler) {
    volatile u8* irFlag = (volatile u8*)0x8000120C;
    u32 limit = (*irFlag != 0) ? RESERVED_SLOT_START : 24;
    for (u32 i = 0; i < limit; i++) {
        if (handler->toSendEntries[i].state == RKNet::EVENTENTRYSTATE_FREE) return true;
    }
    return false;
}
kmBranch(0x8065b8d4, ItemRainHasFreeEntries);

static void OverflowInterceptor(u8* overflowHandler, u32 objId, u32 action, void* data, u32 size) {
    volatile u8* irFlag = (volatile u8*)0x8000120C;
    if (*irFlag != 0) {
        RKNet::EVENTHandler* handler = RKNet::EVENTHandler::sInstance;
        if (handler && handler->isPrepared && Raceinfo::sInstance) {
            ForceFlushExpiredEvents();

            bool isCritical = (objId == OBJ_THUNDER_CLOUD ||
                               objId == OBJ_LIGHTNING ||
                               objId == OBJ_BLOOPER ||
                               objId == OBJ_POW_BLOCK ||
                               objId == OBJ_BOBOMB ||
                               objId == OBJ_BLUE_SHELL ||
                               action == RKNet::EVENTACTION_USE ||
                               action == RKNet::EVENTACTION_TC_LOST ||
                               action == RKNet::EVENTACTION_6);

            bool added = false;
            if (isCritical) {
                if (HasAnyFreeEntry(handler) && handler->freeDataInSendBuffer >= size) {
                    added = true;
                } else if (FlushMushroomEvents() && HasAnyFreeEntry(handler) && handler->freeDataInSendBuffer >= size) {
                    added = true;
                }
            } else {
                if (handler->HasFreeEntries() && handler->freeDataInSendBuffer >= size) {
                    added = true;
                } else if (FlushMushroomEvents() && handler->HasFreeEntries() && handler->freeDataInSendBuffer >= size) {
                    added = true;
                }
            }

            if (added) {
                handler->AddEntry(
                    static_cast<ItemObjId>(objId),
                    static_cast<RKNet::EVENTAction>(action),
                    data, size
                );
                OriginalOverflowAdd(overflowHandler, objId, action, data, size);
                return;
            }
        }
    }
    OriginalOverflowAdd(overflowHandler, objId, action, data, size);
}
kmBranch(0x8079bfec, OverflowInterceptor);

static int SafeGetMovingRoadType(void* colInfo) {
    if (colInfo) {
        void* obj = *(void**)((u32)colInfo + 0x4);
        if (obj) {
            void** vtable = *(void***)obj;
            if (vtable) {
                int (*func)(void*) = (int (*)(void*))vtable[0x104 / 4];
                if (func) return func(obj);
            }
        }
    }
    return 0;
}
kmBranch(0x807bd850, SafeGetMovingRoadType);

static float SafeGetMovingRoadVelocity(void* colInfo) {
    if (colInfo) {
        void* obj = *(void**)((u32)colInfo + 0x4);
        if (obj) {
            void** vtable = *(void***)obj;
            if (vtable) {
                float (*func)(void*) = (float (*)(void*))vtable[0x108 / 4];
                if (func) return func(obj);
            }
        }
    }
    return 0.0f;
}
kmBranch(0x807bd8d4, SafeGetMovingRoadVelocity);

}  // namespace ItemRain
}  // namespace Pulsar