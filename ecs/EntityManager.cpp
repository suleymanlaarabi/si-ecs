#include "EntityManager.hpp"
#include <algorithm>
#include <cstring>
#include "ComponentRegistry.hpp"
#include "EcsAssert.hpp"
#include "TableRegistry.hpp"

namespace {
    constexpr size_t BatchSeenCapacity = static_cast<size_t>(UINT16_MAX) + 1u;

    [[nodiscard]] EntityType mergeEntityTypeWithAdded(const EntityType& currentType,
                                                      const ComponentId* sortedAdded,
                                                      const uint16_t addedCount) {
        ecs_assert(static_cast<uint32_t>(currentType.count) + addedCount <= UINT16_MAX, "entity type limit reached");

        const EntityType merged = {
            .cids = static_cast<ComponentId*>(malloc(sizeof(ComponentId) * (currentType.count + addedCount))),
            .count = static_cast<uint16_t>(currentType.count + addedCount)
        };

        uint16_t currentIndex = 0;
        uint16_t addedIndex = 0;
        uint16_t outIndex = 0;

        while (currentIndex < currentType.count && addedIndex < addedCount) {
            if (currentType.cids[currentIndex] < sortedAdded[addedIndex]) {
                merged.cids[outIndex++] = currentType.cids[currentIndex++];
            } else {
                merged.cids[outIndex++] = sortedAdded[addedIndex++];
            }
        }

        if (currentIndex < currentType.count) {
            std::memcpy(merged.cids + outIndex, currentType.cids + currentIndex,
                        sizeof(ComponentId) * (currentType.count - currentIndex));
            outIndex = static_cast<uint16_t>(outIndex + (currentType.count - currentIndex));
        }

        if (addedIndex < addedCount) {
            std::memcpy(merged.cids + outIndex, sortedAdded + addedIndex,
                        sizeof(ComponentId) * (addedCount - addedIndex));
        }

        return merged;
    }
}

void EntityManager::beginBatchDedupPass() {
    if (++batchSeenGeneration != 0) {
        return;
    }

    std::memset(batchSeenMarks.get(), 0, sizeof(uint32_t) * BatchSeenCapacity);
    batchSeenGeneration = 1;
}

bool EntityManager::markBatchComponentSeen(const ComponentId cid) {
    if (batchSeenMarks[cid] == batchSeenGeneration) {
        return false;
    }

    batchSeenMarks[cid] = batchSeenGeneration;
    return true;
}

void EntityManager::finalizeRowMigration(Table& from, EntityRecord& record, const Entity entity,
                                         const EntityRow newRow) {
    if (const Entity removed = from.removeEntity(record.row); removed.index != 0) {
        this->getEntityRecord(removed).row = record.row;
    }

    record.row = newRow;
}

void EntityManager::migrateEntityRowAdd(Table& from, Table& to, EntityRecord& record, const Entity entity,
                                        const uint16_t insertIndex) {
    const EntityRow newRow = to.addEntity(entity);
    const EntityType& fromType = from.getType();

    for (uint16_t fromIndex = 0; fromIndex < fromType.count; ++fromIndex) {
        const ComponentId cid = fromType.cids[fromIndex];
        const auto toIndex = static_cast<uint16_t>(fromIndex + (fromIndex >= insertIndex));
        if (const size_t size = this->getComponentRecord(cid).size; size != 0) {
            std::memcpy(to.getComponentByIndex(newRow, toIndex), from.getComponentByIndex(record.row, fromIndex), size);
        }
    }

    this->finalizeRowMigration(from, record, entity, newRow);
}

void EntityManager::migrateEntityRow(Table& from, Table& to, EntityRecord& record, const Entity entity) {
    const EntityRow newRow = to.addEntity(entity);

    const EntityType& fromType = from.getType();
    const EntityType& toType = to.getType();

    uint16_t fromIndex = 0;
    uint16_t toIndex = 0;

    while (fromIndex < fromType.count && toIndex < toType.count) {
        const ComponentId fromCid = fromType.cids[fromIndex];

        if (const ComponentId toCid = toType.cids[toIndex]; fromCid == toCid) {
            if (const size_t size = this->getComponentRecord(fromCid).size; size != 0) {
                std::memcpy(
                    to.getComponentByIndex(newRow, toIndex),
                    from.getComponentByIndex(record.row, fromIndex),
                    size
                );
            }

            ++fromIndex;
            ++toIndex;
        } else if (fromCid < toCid) {
            ++fromIndex;
        } else {
            ++toIndex;
        }
    }

    this->finalizeRowMigration(from, record, entity, newRow);
}

void EntityManager::migrateEntityRowRemove(Table& from, Table& to, EntityRecord& record, const Entity entity,
                                           const uint16_t removeIndex) {
    const EntityRow newRow = to.addEntity(entity);
    const EntityType& toType = to.getType();

    for (uint16_t toIndex = 0; toIndex < toType.count; ++toIndex) {
        const ComponentId cid = toType.cids[toIndex];
        const auto fromIndex = static_cast<uint16_t>(toIndex + (toIndex >= removeIndex));
        if (const size_t size = this->getComponentRecord(cid).size; size != 0) {
            std::memcpy(to.getComponentByIndex(newRow, toIndex), from.getComponentByIndex(record.row, fromIndex), size);
        }
    }

    this->finalizeRowMigration(from, record, entity, newRow);
}

void EntityManager::addBatchComponents(const Entity entity, const ComponentId* cids, uint32_t cidCount) {
    ecs_assert_entity_alive(entity);

    EntityRecord& record = this->getEntityRecord(entity);
    const TableId currentTid = record.tid;
    const Table& currentTable = this->getTables()[currentTid];
    const EntityType& currentType = currentTable.getType();

    tmpAllocator.release();
    uint32_t addedCapacity = 0;

    for (uint32_t i = 0; i < cidCount; ++i) {
        const ComponentId cid = cids[i];
        if (currentTable.hasComponent(cid)) {
            continue;
        }
        addedCapacity += 1;
        addedCapacity += this->getComponentRecord(cid).required.size;
    }

    if (addedCapacity == 0) {
        return;
    }

    auto* addedStorage = static_cast<ComponentId*>(tmpAllocator.allocate(sizeof(ComponentId) * addedCapacity * 2));
    auto* addedCids = addedStorage;
    auto* sortedAdded = addedStorage + addedCapacity;
    uint32_t addedCount = 0;

    this->beginBatchDedupPass();
    auto queueAddedCid = [&](const ComponentId cid) {
        if (currentTable.hasComponent(cid) || !this->markBatchComponentSeen(cid)) {
            return;
        }

        addedCids[addedCount++] = cid;
    };

    for (uint32_t i = 0; i < cidCount; ++i) {
        const ComponentId cid = cids[i];
        queueAddedCid(cid);
        if (const ComponentRecord& componentRecord = this->getComponentRecord(cid); !componentRecord.required.empty()) {
            for (const ComponentId addCid : componentRecord.required) {
                queueAddedCid(addCid);
            }
        }
    }

    if (addedCount == 0) {
        tmpAllocator.release();
        return;
    }

    std::memcpy(sortedAdded, addedCids, sizeof(ComponentId) * addedCount);
    std::sort(sortedAdded, sortedAdded + addedCount);

    EntityType type = mergeEntityTypeWithAdded(currentType, sortedAdded, static_cast<uint16_t>(addedCount));
    const TableId newTid = this->findOrCreateTable(std::move(type), *this).first;

    this->migrateEntityRow(this->getTables()[currentTid], this->getTables()[newTid], record, entity);
    record.tid = newTid;

    for (uint32_t i = 0; i < addedCount; ++i) {
        if (const ComponentRecord& componentRecord = this->getComponentRecord(addedCids[i]); componentRecord.onAdd) {
            componentRecord.onAdd(entity, this);
        }
    }

    tmpAllocator.release();
}


void EntityManager::addComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");

    EntityRecord& record = this->getEntityRecord(entity);
    const TableId currentTid = record.tid;
    Table* currentTable = &this->getTables()[currentTid];

    if (currentTable->hasComponent(cid)) {
        return;
    }

    const ComponentRecord& componentRecord = this->getComponentRecord(cid);
    TableId newTid;

    if (currentTable->hasAddEdge(cid)) {
        newTid = currentTable->getAddEdge(cid);
    } else {
        EntityType type = currentTable->getType().withAdd(cid);
        if (!componentRecord.required.empty()) {
            for (const ComponentId addCid : componentRecord.required) {
                if (!currentTable->hasComponent(addCid)) {
                    type.add(addCid);
                }
            }
        }

        newTid = this->findOrCreateTable(std::move(type), *this).first;
        currentTable = &this->getTables()[currentTid];
        currentTable->setAddEdge(cid, newTid);
    }


    Table& newTable = this->getTables()[newTid];
    if (componentRecord.required.empty()) {
        const uint16_t insertIndex = newTable.getType().findIndex(cid);
        this->migrateEntityRowAdd(*currentTable, newTable, record, entity, insertIndex);
        record.tid = newTid;
    } else {
        this->migrateEntityRow(*currentTable, newTable, record, entity);
        record.tid = newTid;
        for (const ComponentId addCid : componentRecord.required) {
            if (currentTable->hasComponent(addCid)) {
                continue;
            }

            if (const ComponentRecord& requiredRecord = this->getComponentRecord(addCid);
                requiredRecord.onAdd) {
                requiredRecord.onAdd(entity, this);
            }
        }
    }

    if (componentRecord.onAdd) {
        componentRecord.onAdd(entity, this);
    }
}

void EntityManager::destroyEntity(const Entity entity) {
    ecs_assert_entity_alive(entity);

    EntityRecord& record = this->getEntityRecord(entity);
    const TableId currentTid = record.tid;
    Table& currentTable = this->getTables()[currentTid];
    const auto& [cids, count] = currentTable.getType();

    for (uint16_t i = 0; i < count; ++i) {
        if (const ComponentRecord& componentRecord = this->getComponentRecord(cids[i]);
            componentRecord.onRemove) {
            componentRecord.onRemove(entity, this);
        }
    }

    if (count != 0) {
        if (const Entity swapped = currentTable.removeEntity(record.row); swapped.index != 0) {
            this->getEntityRecord(swapped).row = record.row;
        }
    }

    record.tid = 0;
    record.row = 0;
    EntityRegistry::destroyEntity(entity);
}

void* EntityManager::getComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");

    const EntityRecord& record = this->getEntityRecord(entity);
    const Table& table = this->getTables()[record.tid];

    return table.getComponent(record.row, cid);
}

void EntityManager::setComponent(const Entity entity, const ComponentId cid, const void* component) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");

    const ComponentRecord& componentRecord = this->getComponentRecord(cid);
    if (componentRecord.onSet) {
        componentRecord.onSet(entity, component, this);
    }

    this->addComponent(entity, cid);

    const EntityRecord& record = this->getEntityRecord(entity);
    const Table& table = this->getTables()[record.tid];

    void* value = table.getComponent(record.row, cid);

    memcpy(value, component, componentRecord.size);
}


void EntityManager::removeComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");

    EntityRecord& record = this->getEntityRecord(entity);
    const TableId currentTid = record.tid;
    Table* currentTable = &this->getTables()[currentTid];
    const EntityType& currentType = currentTable->getType();
    const uint16_t removeIndex = currentType.findIndex(cid);

    if (removeIndex == UINT16_MAX) {
        return;
    }

    if (const ComponentRecord& componentRecord = this->getComponentRecord(cid);
        componentRecord.onRemove) {
        componentRecord.onRemove(entity, this);
    }
    TableId newTid;

    if (currentTable->hasRemoveEdge(cid)) {
        newTid = currentTable->getRemoveEdge(cid);
    } else {
        EntityType type = currentType.withRemove(cid);
        newTid = this->findOrCreateTable(std::move(type), *this).first;
        currentTable = &this->getTables()[currentTid];
        currentTable->setRemoveEdge(cid, newTid);
    }

    this->migrateEntityRowRemove(*currentTable,
                                 this->getTables()[newTid],
                                 record, entity, removeIndex);
    record.tid = newTid;
}

bool EntityManager::hasComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");
    if (const EntityRecord& record = this->getEntityRecord(entity); this->getTables()[record.tid].
                                                                    getType().
                                                                    findIndex(cid) ==
        UINT16_MAX) {
        return false;
    }
    return true;
}

std::pair<Table&, EntityRow> EntityManager::getTable(const Entity entity) {
    EntityRecord& record = this->getEntityRecord(entity);
    return {
        this->getTables()[record.tid],
        record.row
    };
}
