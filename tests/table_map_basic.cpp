#include <criterion/criterion.h>

#include <initializer_list>
#include <vector>

#include "ecs/core/component/ComponentRegistry.hpp"
#include "ecs/core/base/EcsType.hpp"
#include "ecs/core/storage/TableMap.hpp"

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
    for (const ComponentId id: ids) {
        type.add(id);
    }
    return type;
}

static std::vector<EntityType> makeCandidateTypes(const std::vector<ComponentId> &ids) {
    std::vector<EntityType> candidates;

    candidates.push_back(makeType({}));
    for (const ComponentId id: ids) {
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
    auto [fst, firstTable] = map.findOrCreate(EntityType{}, registry, firstCreated);

    bool secondCreated = false;
    std::pair<TableId, Table &> second = map.findOrCreate(EntityType{}, registry, secondCreated);

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
    bool firstCreated = false;
    const TableId first = map.findOrCreate(makeType({positionId, velocityId}), registry, firstCreated).first;
    bool secondCreated = false;
    const TableId second = map.findOrCreate(makeType({positionId, velocityId}), registry, secondCreated).first;

    cr_assert_eq(first, second);
    cr_assert(firstCreated);
    cr_assert_not(secondCreated);

    const ComponentRecord &positionRecord = registry.getComponentRecord(positionId);
    const ComponentRecord &velocityRecord = registry.getComponentRecord(velocityId);

    cr_assert_eq(positionRecord.tables.size, 1);
    cr_assert_eq(velocityRecord.tables.size, 1);
    cr_assert_eq(positionRecord.tables[0], first);
    cr_assert_eq(velocityRecord.tables[0], first);
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

    for (const EntityType &type: insertedTypes) {
        bool isCreated = false;
        assignedIds.push_back(map.findOrCreate(type.clone(), registry, isCreated).first);
        cr_assert(isCreated);
    }

    for (size_t i = 0; i < insertedTypes.size(); ++i) {
        bool isCreated = false;
        cr_assert_eq(assignedIds[i], i);
        cr_assert(map.contains(insertedTypes[i]));
        cr_assert_eq(map.findOrCreate(insertedTypes[i].clone(), registry, isCreated).first, assignedIds[i]);
        cr_assert_not(isCreated);
    }

    for (EntityType &type: insertedTypes) {
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
    const TableId positionTable = map.findOrCreate(positionType.clone(), registry, positionCreated).first;

    const uint64_t velocityHash = hashEntityType(velocityType);
    const uint32_t velocityIndex = velocityHash & (map.size - 1u);

    map.buckets[velocityIndex].tableId = positionTable;
    map.buckets[velocityIndex].hash = velocityHash;

    bool velocityCreated = false;
    const TableId velocityTable = map.findOrCreate(velocityType.clone(), registry, velocityCreated).first;

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


Test(table_map, growth_updates_bucket_indices) {
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
    EntityType *targetType = nullptr;

    for (EntityType &candidate: candidates) {
        if ((hashEntityType(candidate) & 15u) != (hashEntityType(candidate) & 31u)) {
            targetType = &candidate;
            break;
        }
    }

    cr_assert_not_null(targetType);

    TableMap map;
    bool targetCreated = false;
    (void) map.findOrCreate(targetType->clone(), registry, targetCreated);

    cr_assert(targetCreated);

    uint32_t inserted = 1;
    for (EntityType &candidate: candidates) {
        if (&candidate == targetType) {
            continue;
        }

        bool isCreated = false;
        (void) map.findOrCreate(candidate.clone(), registry, isCreated);
        inserted += 1;

        if (inserted >= 9) {
            break;
        }
    }

    cr_assert_eq(map.size, 32);
    for (TableId tid = 0; tid < map.tables.size(); ++tid) {
        uint32_t actualBucketIndex = UINT32_MAX;
        for (uint32_t bucketIndex = 0; bucketIndex < map.size; ++bucketIndex) {
            if (map.buckets[bucketIndex].tableId == tid) {
                actualBucketIndex = bucketIndex;
                break;
            }
        }

        cr_assert_neq(actualBucketIndex, UINT32_MAX);
    }

    for (EntityType &candidate: candidates) {
        candidate.release();
    }
}

Test(table_map, growth_preserves_table_ids_after_vector_reallocation) {
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

    std::vector<EntityType> insertedTypes = makeCandidateTypes(ids);
    TableMap map;

    for (EntityType& type : insertedTypes) {
        bool isCreated = false;
        const auto [tid, table] = map.findOrCreate(type.clone(), registry, isCreated);
        cr_assert(isCreated);
        cr_assert_eq(table.id, tid);
    }

    for (TableId tid = 0; tid < map.tables.size(); ++tid) {
        cr_assert_eq(map.tables[tid]->id, tid);
    }

    for (EntityType& type : insertedTypes) {
        type.release();
    }
}
