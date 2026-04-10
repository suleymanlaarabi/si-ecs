#include <sstream>
#include "EntityManager.hpp"
#include <cstring>
#include "ComponentRegistry.hpp"
#include "EcsAssert.hpp"
#include "EcsType.hpp"
#include "Table.hpp"
#include "JitCompiler.hpp"

void EntityManager::finalizeRowMigration(Table& from, EntityRecord& record,
                                         const EntityRow newRow) {
    if (const Entity removed = from.removeEntity(record.row); removed.index != 0) {
        this->getEntityRecord(removed).row = record.row;
    }

    record.row = newRow;
}

ResolvedTableEdge EntityManager::resolveAddEdge(const TableId fromTid, const ComponentId cid) {
    Table& from = this->tables_map.tables[fromTid];
    if (TableEdge* edge = from.edges.add.tryGet(cid);
        edge != nullptr && edge->tid != InvalidTableId) {
        return {
            .from = &from,
            .to = &this->tables_map.tables[edge->tid],
            .edge = edge,
        };
    }

    EntityType type = from.getType().withAdd(cid);
    const TableId toTid = this->findOrCreateTable(std::move(type), *this).first;

    Table& stableFrom = this->tables_map.tables[fromTid];
    stableFrom.edges.add.set(cid, TableEdge{.tid = toTid});
    return {
        .from = &stableFrom,
        .to = &this->tables_map.tables[toTid],
        .edge = &stableFrom.edges.add.at(cid),
    };
}

ResolvedTableEdge EntityManager::resolveRemoveEdge(const TableId fromTid, const ComponentId cid) {
    Table& from = this->tables_map.tables[fromTid];
    if (TableEdge* edge = from.edges.remove.tryGet(cid);
        edge != nullptr && edge->tid != InvalidTableId) {
        return {
            .from = &from,
            .to = &this->tables_map.tables[edge->tid],
            .edge = edge,
        };
    }

    EntityType type = from.type.withRemove(cid);
    const TableId toTid = this->findOrCreateTable(std::move(type), *this).first;

    Table& stableFrom = this->tables_map.tables[fromTid];
    stableFrom.edges.remove.set(cid, TableEdge{.tid = toTid});
    return {
        .from = &stableFrom,
        .to = &this->tables_map.tables[toTid],
        .edge = &stableFrom.edges.remove.at(cid),
    };
}

static std::string generateMigrationCode(const Table& from, const Table& to, ComponentRegistry& registry, const std::string& funcName) {
    std::stringstream ss;
    ss << "struct Column { unsigned long long size; void* data; };\n";
    ss << "typedef struct { int a, b, c; } s12;\n";
    ss << "void " << funcName << "(struct Column* src_cols, struct Column* dst_cols, unsigned int src_row, unsigned int dst_row) {\n";

    const EntityType& fromType = from.getType();
    const EntityType& toType = to.getType();

    uint16_t fromIndex = 0;
    uint16_t toIndex = 0;

    while (fromIndex < fromType.count && toIndex < toType.count) {
        const ComponentId fromCid = fromType.cids[fromIndex];
        const ComponentId toCid = toType.cids[toIndex];

        if (fromCid == toCid) {
            const size_t size = registry.getComponentRecord(fromCid).size;
            if (size > 0) {
                // Use a specialized assignment if size is a multiple of 4 or 8
                if (size == 4) ss << "  *(int*)((char*)dst_cols[" << toIndex << "].data + dst_row * 4) = *(int*)((char*)src_cols[" << fromIndex << "].data + src_row * 4);\n";
                else if (size == 8) ss << "  *(long long*)((char*)dst_cols[" << toIndex << "].data + dst_row * 8) = *(long long*)((char*)src_cols[" << fromIndex << "].data + src_row * 8);\n";
                else if (size == 12) {
                    ss << "  *(s12*)((char*)dst_cols[" << toIndex << "].data + dst_row * 12) = *(s12*)((char*)src_cols[" << fromIndex << "].data + src_row * 12);\n";
                }
                else {
                    ss << "  __builtin_memcpy((char*)dst_cols[" << toIndex << "].data + dst_row * " << size 
                       << ", (char*)src_cols[" << fromIndex << "].data + src_row * " << size 
                       << ", " << size << ");\n";
                }
            }
            ++fromIndex;
            ++toIndex;
        } else if (fromCid < toCid) {
            ++fromIndex;
        } else {
            ++toIndex;
        }
    }

    ss << "}\n";
    return ss.str();
}

JitMigrationFn EntityManager::compileMigration(const Table& from, const Table& to) {
    const std::string funcName = "migrate_" + std::to_string(from.id) + "_to_" + std::to_string(to.id);
    const std::string code = generateMigrationCode(from, to, *this, funcName);
    return reinterpret_cast<JitMigrationFn>(JitCompiler::instance().compileMigration(code, funcName));
}

void EntityManager::migrateEntityRow(Table& from, Table& to, EntityRecord& record, const Entity entity,
                                     const JitMigrationFn migration) {
    const EntityRow newRow = to.addEntity(entity);
    migration(from.columns, to.columns, record.row, newRow);

    this->finalizeRowMigration(from, record, newRow);
}


void EntityManager::addComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");

    EntityRecord& record = this->getEntityRecord(entity);
    const ResolvedTableEdge resolved = this->resolveAddEdge(record.tid, cid);
    if (resolved.edge->tid == record.tid) {
        return;
    }

    if (resolved.edge->migration == nullptr) {
        resolved.edge->migration = this->compileMigration(*resolved.from, *resolved.to);
    }

    this->migrateEntityRow(*resolved.from, *resolved.to, record, entity, resolved.edge->migration);
    record.tid = resolved.edge->tid;

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
    Table& from = this->getTables()[record.tid];

    if (!from.hasComponent(cid)) {
        return;
    }

    if (const ComponentRecord& componentRecord = this->getComponentRecord(cid); componentRecord.onRemove) {
        componentRecord.onRemove(entity, this);
    }

    const ResolvedTableEdge resolved = this->resolveRemoveEdge(record.tid, cid);
    if (resolved.edge->migration == nullptr) {
        resolved.edge->migration = this->compileMigration(*resolved.from, *resolved.to);
    }

    this->migrateEntityRow(*resolved.from, *resolved.to, record, entity, resolved.edge->migration);
    record.tid = resolved.edge->tid;
}

bool EntityManager::hasComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");
    const EntityRecord& record = this->getEntityRecord(entity);
    return this->tables_map.tables[record.tid].hasComponent(cid);
}

std::pair<Table&, EntityRow> EntityManager::getTable(const Entity entity) {
    EntityRecord& record = this->getEntityRecord(entity);
    return {
        this->getTables()[record.tid],
        record.row
    };
}
