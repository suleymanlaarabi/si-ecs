#include "EntityManager.hpp"
#include <cstring>
#include "ComponentRegistry.hpp"
#include "EcsAssert.hpp"
#include "EcsType.hpp"
#include "Table.hpp"

void EntityManager::finalizeRowMigration(Table& from, EntityRecord& record,
                                         const EntityRow newRow) {
    if (const Entity removed = from.removeEntity(record.row); removed.index != 0) {
        this->getEntityRecord(removed).row = record.row;
    }

    record.row = newRow;
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

    this->finalizeRowMigration(from, record, newRow);
}


void EntityManager::addComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");

    EntityRecord& record = this->getEntityRecord(entity);
    const TableId currentTid = record.tid;
    Table* currentTable = &this->tables_map.tables[currentTid];

    TableId newTid;
    const uint16_t edge = currentTable->edges.add.atOrInvalid(cid);
    if (edge == currentTid) return;
    if (edge != UINT16_MAX) {
        newTid = edge;
    } else {
        EntityType type = currentTable->getType().withAdd(cid);
        newTid = this->findOrCreateTable(std::move(type), *this).first;
        currentTable = &this->tables_map.tables[currentTid];
        currentTable->setAddEdge(cid, newTid);
    }

    Table& newTable = this->tables_map.tables[newTid];
    this->migrateEntityRow(*currentTable, newTable, record, entity);
    record.tid = newTid;

    const ComponentRecord& componentRecord = this->getComponentRecord(cid);

    if (componentRecord.onAdd) {
        componentRecord.onAdd(entity, this);
    }
}

void EntityManager::destroyEntity(const Entity entity) {
    ecs_assert_entity_alive(entity);

    EntityRecord& record = this->getEntityRecord(entity);
    const TableId currentTid = record.tid;
    Table& currentTable = this->tables_map.tables[currentTid];
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

    if (!currentTable->hasComponent(cid)) {
        return;
    }

    if (const ComponentRecord& componentRecord = this->getComponentRecord(cid); componentRecord.onRemove) {
        componentRecord.onRemove(entity, this);
    }
    TableId newTid;

    if (currentTable->hasRemoveEdge(cid)) {
        newTid = currentTable->getRemoveEdge(cid);
    } else {
        EntityType type = currentTable->type.withRemove(cid);
        newTid = this->findOrCreateTable(std::move(type), *this).first;
        currentTable = &this->getTables()[currentTid];
        currentTable->setRemoveEdge(cid, newTid);
    }

    this->migrateEntityRow(*currentTable, this->getTables()[newTid], record, entity);
    record.tid = newTid;
}

bool EntityManager::hasComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");
    if (const EntityRecord& record = this->getEntityRecord(entity); this->tables_map.tables[record.tid].
        hasComponent(cid, record.tid)) {
        return true;
    }
    return false;
}

std::pair<Table&, EntityRow> EntityManager::getTable(const Entity entity) {
    EntityRecord& record = this->getEntityRecord(entity);
    return {
        this->getTables()[record.tid],
        record.row
    };
}
