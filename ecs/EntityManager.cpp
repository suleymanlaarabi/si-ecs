#include "EntityManager.hpp"
#include <cstring>
#include "ComponentRegistry.hpp"
#include "EcsAssert.hpp"
#include "TableRegistry.hpp"

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

    EntityType type = currentTable.getType().clone();

    tmpAllocator.release();
    uint addedCount = 0;
    auto pushAddedRecord = [&](ComponentRecord& componentRecord) {
        *static_cast<ComponentRecord**>(tmpAllocator.allocate(sizeof(ComponentRecord*))) = &componentRecord;
        addedCount += 1;
    };

    for (uint i = 0; i < cidCount; i++) {
        if (type.findIndex(cids[i]) != UINT16_MAX) {
            continue;
        }
        type.add(cids[i]);
        ComponentRecord& componentRecord = this->getComponentRecord(cids[i]);
        pushAddedRecord(componentRecord);
        if (!componentRecord.required.empty()) {
            for (const ComponentId addCid : componentRecord.required) {
                if (type.findIndex(addCid) != UINT16_MAX) {
                    continue;
                }
                type.add(addCid);
                pushAddedRecord(this->getComponentRecord(addCid));
            }
        }
    }

    if (addedCount == 0) {
        type.release();
        tmpAllocator.release();
        return;
    }

    bool isCreated = false;
    const TableId newTid = this->findOrCreateTable(type, *this, isCreated).first;
    if (!isCreated) {
        type.release();
    }

    this->migrateEntityRow(this->getTables()[currentTid], this->getTables()[newTid], record, entity);
    record.tid = newTid;

    const auto records = reinterpret_cast<ComponentRecord**>(tmpAllocator.data);
    for (uint i = 0; i < addedCount; i++) {
        if (records[i]->onAdd) {
            records[i]->onAdd(entity, this);
        }
    }


    tmpAllocator.release();
}


void EntityManager::addComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");

    EntityRecord& record = this->getEntityRecord(entity);
    const TableId currentTid = record.tid;
    const Table& currentTable = this->getTables()[currentTid];

    if (currentTable.hasComponent(cid)) {
        return;
    }

    const ComponentRecord& componentRecord = this->getComponentRecord(cid);

    EntityType type = currentTable.getType().withAdd(cid);
    if (!componentRecord.required.empty()) {
        for (const ComponentId addCid : componentRecord.required) {
            if (!currentTable.hasComponent(addCid)) {
                type.add(addCid);
            }
        }
    }

    const uint16_t insertIndex = type.findIndex(cid);
    const TableId newTid = this->findOrCreateTable(type, *this).first;

    this->migrateEntityRowAdd(this->getTables()[currentTid], this->getTables()[newTid],
                              record, entity, insertIndex);
    record.tid = newTid;

    if (componentRecord.onAdd) {
        componentRecord.onAdd(entity, this);
    }

    for (const ComponentId addCid : componentRecord.required) {
        if (this->getTables()[currentTid].hasComponent(addCid)) {
            continue;
        }

        if (const ComponentRecord& requiredRecord = this->getComponentRecord(addCid);
            requiredRecord.onAdd) {
            requiredRecord.onAdd(entity, this);
        }
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
    const EntityType& currentType = this->getTables()[currentTid].getType();
    const uint16_t removeIndex = currentType.findIndex(cid);

    if (removeIndex == UINT16_MAX) {
        return;
    }

    if (const ComponentRecord& componentRecord = this->getComponentRecord(cid);
        componentRecord.onRemove) {
        componentRecord.onRemove(entity, this);
    }
    const EntityType type = currentType.withRemove(cid);
    const TableId newTid = this->findOrCreateTable(type, *this).first;

    this->migrateEntityRowRemove(this->getTables()[currentTid],
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
