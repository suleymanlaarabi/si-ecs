#include <criterion/criterion.h>

#include <initializer_list>
#include <vector>

#include "../ecs/ComponentRegistry.hpp"
#include "../ecs/EcsType.hpp"
#include "../ecs/TableMap.hpp"

struct TableMapPosition {
    int x;
    int y;
};

struct TableMapVelocity {
    int dx;
    int dy;
};

struct TableMapHealth {
    int value;
};

struct TableMapMana {
    int value;
};

struct TableMapArmor {
    int value;
};

struct TableMapTag {};

static EntityType makeType(const std::initializer_list<ComponentId> ids) {
    EntityType type{};
    for (const ComponentId id : ids) {
        type.add(id);
    }
    return type;
}

static std::vector<EntityType> makeCandidateTypes(const std::vector<ComponentId>& ids) {
    std::vector<EntityType> candidates;

    candidates.push_back(makeType({}));
    for (const ComponentId id : ids) {
        candidates.push_back(makeType({id}));
    }

    for (size_t i = 0; i < ids.size(); ++i) {
        for (size_t j = i + 1; j < ids.size(); ++j) {
            candidates.push_back(makeType({ids[i], ids[j]}));
        }
    }

    return candidates;
}

Test(table_map, find_or_create_reuses_existing_table_for_same_type) {
    ComponentRegistry registry;
    registry.registerComponent<TableMapPosition>();

    TableMap map;
    constexpr EntityType emptyType{};

    cr_assert_not(map.contains(emptyType));

    bool firstCreated = false;
    auto [fst, firstTable] = map.findOrCreate(emptyType, registry, firstCreated);

    bool secondCreated = false;
    std::pair<TableId, Table&> second = map.findOrCreate(emptyType, registry, secondCreated);

    cr_assert_eq(fst, 0);
    cr_assert_eq(second.first, fst);
    cr_assert(firstCreated);
    cr_assert_not(secondCreated);
    cr_assert(map.contains(emptyType));
}

Test(table_map, duplicate_type_does_not_create_duplicate_component_table_entries) {
    ComponentRegistry registry;
    registry.registerComponent<TableMapPosition>();
    registry.registerComponent<TableMapVelocity>();

    const ComponentId positionId = ComponentRegistry::id<TableMapPosition>();
    const ComponentId velocityId = ComponentRegistry::id<TableMapVelocity>();

    TableMap map;
    EntityType movingType = makeType({positionId, velocityId});

    bool firstCreated = false;
    const TableId first = map.findOrCreate(movingType, registry, firstCreated).first;
    bool secondCreated = false;
    const TableId second = map.findOrCreate(movingType, registry, secondCreated).first;

    cr_assert_eq(first, second);
    cr_assert(firstCreated);
    cr_assert_not(secondCreated);

    const ComponentRecord& positionRecord = registry.getComponentRecord(positionId);
    const ComponentRecord& velocityRecord = registry.getComponentRecord(velocityId);

    cr_assert_eq(positionRecord.tables.size, 1);
    cr_assert_eq(velocityRecord.tables.size, 1);
    cr_assert_eq(positionRecord.tables[0], first);
    cr_assert_eq(velocityRecord.tables[0], first);

    movingType.release();
}

Test(table_map, growth_preserves_existing_type_lookups) {
    ComponentRegistry registry;
    registry.registerComponent<TableMapPosition>();
    registry.registerComponent<TableMapVelocity>();
    registry.registerComponent<TableMapHealth>();
    registry.registerComponent<TableMapMana>();
    registry.registerComponent<TableMapArmor>();
    registry.registerComponent<TableMapTag>();

    const ComponentId positionId = ComponentRegistry::id<TableMapPosition>();
    const ComponentId velocityId = ComponentRegistry::id<TableMapVelocity>();
    const ComponentId healthId = ComponentRegistry::id<TableMapHealth>();
    const ComponentId manaId = ComponentRegistry::id<TableMapMana>();
    const ComponentId armorId = ComponentRegistry::id<TableMapArmor>();
    const ComponentId tagId = ComponentRegistry::id<TableMapTag>();

    std::vector<EntityType> insertedTypes;
    insertedTypes.reserve(20);

    insertedTypes.push_back(makeType({}));
    insertedTypes.push_back(makeType({positionId}));
    insertedTypes.push_back(makeType({velocityId}));
    insertedTypes.push_back(makeType({healthId}));
    insertedTypes.push_back(makeType({manaId}));
    insertedTypes.push_back(makeType({armorId}));
    insertedTypes.push_back(makeType({tagId}));
    insertedTypes.push_back(makeType({positionId, velocityId}));
    insertedTypes.push_back(makeType({positionId, healthId}));
    insertedTypes.push_back(makeType({positionId, manaId}));
    insertedTypes.push_back(makeType({positionId, armorId}));
    insertedTypes.push_back(makeType({positionId, tagId}));
    insertedTypes.push_back(makeType({velocityId, healthId}));
    insertedTypes.push_back(makeType({velocityId, manaId}));
    insertedTypes.push_back(makeType({velocityId, armorId}));
    insertedTypes.push_back(makeType({velocityId, tagId}));
    insertedTypes.push_back(makeType({healthId, manaId}));
    insertedTypes.push_back(makeType({healthId, armorId}));
    insertedTypes.push_back(makeType({healthId, tagId}));
    insertedTypes.push_back(makeType({manaId, armorId}));

    TableMap map;
    std::vector<TableId> assignedIds;
    assignedIds.reserve(insertedTypes.size());

    for (const EntityType& type : insertedTypes) {
        bool isCreated = false;
        assignedIds.push_back(map.findOrCreate(type, registry, isCreated).first);
        cr_assert(isCreated);
    }

    for (size_t i = 0; i < insertedTypes.size(); ++i) {
        bool isCreated = false;
        cr_assert_eq(assignedIds[i], i);
        cr_assert(map.contains(insertedTypes[i]));
        cr_assert_eq(map.findOrCreate(insertedTypes[i], registry, isCreated).first, assignedIds[i]);
        cr_assert_not(isCreated);
    }

    for (EntityType& type : insertedTypes) {
        type.release();
    }
}

Test(table_map, find_or_create_must_not_reuse_table_when_only_hash_matches) {
    ComponentRegistry registry;
    registry.registerComponent<TableMapPosition>();
    registry.registerComponent<TableMapVelocity>();

    const ComponentId positionId = ComponentRegistry::id<TableMapPosition>();
    const ComponentId velocityId = ComponentRegistry::id<TableMapVelocity>();

    EntityType positionType = makeType({positionId});
    EntityType velocityType = makeType({velocityId});

    TableMap map;
    bool positionCreated = false;
    const TableId positionTable = map.findOrCreate(positionType, registry, positionCreated).first;

    const uint64_t velocityHash = hashEntityType(velocityType);
    const uint32_t velocityIndex = velocityHash & (map.size - 1u);

    map.buckets[velocityIndex].tableId = positionTable;
    map.buckets[velocityIndex].hash = velocityHash;

    bool velocityCreated = false;
    const TableId velocityTable = map.findOrCreate(velocityType, registry, velocityCreated).first;

    cr_assert_neq(
        velocityTable,
        positionTable,
        "findOrCreate should create a new table when a bucket hash matches but the stored type differs"
    );
    cr_assert(velocityCreated);
    cr_assert(map.contains(velocityType));

    positionType.release();
    velocityType.release();
}

Test(table_map, remove_last_table_erases_type_from_lookup) {
    ComponentRegistry registry;
    registry.registerComponent<TableMapPosition>();

    const ComponentId positionId = ComponentRegistry::id<TableMapPosition>();
    EntityType positionType = makeType({positionId});

    TableMap map;
    bool created = false;
    const TableId tid = map.findOrCreate(positionType, registry, created).first;

    cr_assert(created);
    cr_assert(map.contains(positionType));

    const TableId movedTid = map.remove(tid);

    cr_assert_eq(movedTid, InvalidTableId);
    cr_assert_eq(map.tables.size(), 0);
    cr_assert_eq(map.count, 0);
    cr_assert_not(map.contains(positionType));

    positionType.release();
}

Test(table_map, remove_rehashes_cluster_and_keeps_remaining_tables_reachable) {
    ComponentRegistry registry;
    registry.registerComponent<TableMapPosition>();
    registry.registerComponent<TableMapVelocity>();
    registry.registerComponent<TableMapHealth>();
    registry.registerComponent<TableMapMana>();
    registry.registerComponent<TableMapArmor>();
    registry.registerComponent<TableMapTag>();

    const std::vector<ComponentId> ids = {
        ComponentRegistry::id<TableMapPosition>(),
        ComponentRegistry::id<TableMapVelocity>(),
        ComponentRegistry::id<TableMapHealth>(),
        ComponentRegistry::id<TableMapMana>(),
        ComponentRegistry::id<TableMapArmor>(),
        ComponentRegistry::id<TableMapTag>(),
    };

    std::vector<EntityType> candidates = makeCandidateTypes(ids);
    EntityType* firstType = nullptr;
    EntityType* secondType = nullptr;
    uint32_t sharedIndex = 0;

    for (size_t i = 0; i < candidates.size() && firstType == nullptr; ++i) {
        for (size_t j = i + 1; j < candidates.size(); ++j) {
            const uint32_t lhs = hashEntityType(candidates[i]) & 15u;
            const uint32_t rhs = hashEntityType(candidates[j]) & 15u;

            if (lhs == rhs) {
                firstType = &candidates[i];
                secondType = &candidates[j];
                sharedIndex = lhs;
                break;
            }
        }
    }

    cr_assert_not_null(firstType);
    cr_assert_not_null(secondType);

    TableMap map;
    bool firstCreated = false;
    const TableId firstTid = map.findOrCreate(*firstType, registry, firstCreated).first;
    bool secondCreated = false;
    const TableId secondTid = map.findOrCreate(*secondType, registry, secondCreated).first;

    cr_assert(firstCreated);
    cr_assert(secondCreated);
    cr_assert_eq(firstTid, 0);
    cr_assert_eq(secondTid, 1);
    cr_assert_eq(map.tables[firstTid].bucketIndex, sharedIndex);
    cr_assert_eq(map.tables[secondTid].bucketIndex, (sharedIndex + 1) & 15u);

    const TableId movedTid = map.remove(firstTid);

    cr_assert_eq(movedTid, secondTid);
    cr_assert_not(map.contains(*firstType));
    cr_assert(map.contains(*secondType));
    cr_assert_eq(map.tables.size(), 1);
    cr_assert_eq(map.count, 1);
    cr_assert_eq(map.tables[0].bucketIndex, sharedIndex);
    cr_assert_eq(map.buckets[sharedIndex].tableId, 0);

    bool recreated = false;
    cr_assert_eq(map.findOrCreate(*secondType, registry, recreated).first, 0);
    cr_assert_not(recreated);

    for (EntityType& candidate : candidates) {
        candidate.release();
    }
}
