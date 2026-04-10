#include <cstdint>
#include <sstream>
#include "EntityManager.hpp"
#include <cstring>
#include "ComponentRegistry.hpp"
#include "EcsAssert.hpp"
#include "EcsType.hpp"
#include "Table.hpp"
#include "JitCompiler.hpp"

extern "C" EntityRow ecs_table_add_entity(Table* table, const Entity entity) {
    return table->addEntity(entity);
}

extern "C" Entity ecs_table_remove_entity(Table* table, const EntityRow row) {
    return table->removeEntity(row);
}

extern "C" TableId ecs_table_id(const Table* table) {
    return table->id;
}

extern "C" EntityRecord* ecs_entity_record(void* manager, const Entity entity) {
    auto* entityManager = static_cast<EntityManager*>(manager);
    return &entityManager->getEntityRecord(entity);
}

ResolvedTableEdge EntityManager::resolveAddEdge(const TableId fromTid, const ComponentId cid) {
    Table& from = *this->tables_map.tables[fromTid];
    if (TableEdge* edge = from.edges.add.tryGet(cid);
        edge != nullptr && edge->tid != InvalidTableId) {
        return {
            .from = &from,
            .to = this->tables_map.tables[edge->tid],
            .edge = edge,
        };
    }

    EntityType type = from.getType().withAdd(cid);
    const TableId toTid = this->findOrCreateTable(std::move(type), *this).first;

    Table& stableFrom = *this->tables_map.tables[fromTid];
    stableFrom.edges.add.set(cid, TableEdge{.tid = toTid});
    return {
        .from = &stableFrom,
        .to = this->tables_map.tables[toTid],
        .edge = &stableFrom.edges.add.at(cid),
    };
}

ResolvedTableEdge EntityManager::resolveRemoveEdge(const TableId fromTid, const ComponentId cid) {
    Table& from = *this->tables_map.tables[fromTid];
    if (TableEdge* edge = from.edges.remove.tryGet(cid);
        edge != nullptr && edge->tid != InvalidTableId) {
        return {
            .from = &from,
            .to = this->tables_map.tables[edge->tid],
            .edge = edge,
        };
    }

    EntityType type = from.type.withRemove(cid);
    const TableId toTid = this->findOrCreateTable(std::move(type), *this).first;

    Table& stableFrom = *this->tables_map.tables[fromTid];
    stableFrom.edges.remove.set(cid, TableEdge{.tid = toTid});
    return {
        .from = &stableFrom,
        .to = this->tables_map.tables[toTid],
        .edge = &stableFrom.edges.remove.at(cid),
    };
}

static std::string generateMigrationCode(const Table& from, const Table& to, ComponentRegistry& registry,
                                         const std::string& funcName,
                                         void (*hook)(Entity, void*),
                                         const bool hookBeforeFinalize) {
    std::stringstream ss;
    ss << "struct Column { unsigned long long size; void* data; };\n";
    ss << "typedef struct { int a, b, c; } s12;\n";
    ss << "struct Entity { unsigned int index; unsigned short generation; };\n";
    ss <<
        "struct EntityRecord { unsigned short tid; unsigned short generation; unsigned int row; unsigned short listenersId; };\n";
    ss << "struct Table;\n";
    ss << "unsigned int ecs_table_add_entity(struct Table* table, struct Entity entity);\n";
    ss << "struct Entity ecs_table_remove_entity(struct Table* table, unsigned int row);\n";
    ss << "unsigned short ecs_table_id(struct Table* table);\n";
    ss << "struct EntityRecord* ecs_entity_record(void* manager, struct Entity entity);\n";
    ss << "void " << funcName <<
        "(struct Column* src_cols, struct Column* dst_cols, void* manager, struct Table* from, struct Table* to, struct EntityRecord* record, struct Entity entity) {\n";
    ss << "  const unsigned int src_row = record->row;\n";
    if (hook != nullptr && hookBeforeFinalize) {
        ss << "  ((void(*)(struct Entity, void*))" << reinterpret_cast<uintptr_t>(hook) << ")(entity, manager);\n";
    }
    ss << "  const unsigned int dst_row = ecs_table_add_entity(to, entity);\n";

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
                if (size == 4)
                    ss << "  *(int*)((char*)dst_cols[" << toIndex <<
                        "].data + dst_row * 4) = *(int*)((char*)src_cols[" << fromIndex << "].data + src_row * 4);\n";
                else if (size == 8)
                    ss << "  *(long long*)((char*)dst_cols[" << toIndex <<
                        "].data + dst_row * 8) = *(long long*)((char*)src_cols[" << fromIndex <<
                        "].data + src_row * 8);\n";
                else if (size == 12) {
                    ss << "  *(s12*)((char*)dst_cols[" << toIndex << "].data + dst_row * 12) = *(s12*)((char*)src_cols["
                        << fromIndex << "].data + src_row * 12);\n";
                } else {
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

    ss << "  const struct Entity swapped = ecs_table_remove_entity(from, src_row);\n";
    ss << "  if (swapped.index != 0) {\n";
    ss << "    ecs_entity_record(manager, swapped)->row = src_row;\n";
    ss << "  }\n";
    ss << "  record->tid = ecs_table_id(to);\n";
    ss << "  record->row = dst_row;\n";
    if (hook != nullptr && !hookBeforeFinalize) {
        ss << "  ((void(*)(struct Entity, void*))" << reinterpret_cast<uintptr_t>(hook) << ")(entity, manager);\n";
    }
    ss << "}\n";
    return ss.str();
}

JitMigrationFn EntityManager::compileMigration(const Table& from, const Table& to,
                                               void (*hook)(Entity, void*),
                                               const bool hookBeforeFinalize) {
    const std::string funcName = "migrate_" + std::to_string(from.id) + "_to_" + std::to_string(to.id);
    const std::string code = generateMigrationCode(from, to, *this, funcName, hook, hookBeforeFinalize);
    return reinterpret_cast<JitMigrationFn>(JitCompiler::instance().compileMigration(code, funcName));
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
        const ComponentRecord& componentRecord = this->getComponentRecord(cid);
        resolved.edge->migration = this->compileMigration(
            *resolved.from, *resolved.to, componentRecord.onAdd, false);
    }

    resolved.edge->migration(
        resolved.from->columns,
        resolved.to->columns,
        this,
        resolved.from,
        resolved.to,
        &record,
        entity);
}

void EntityManager::destroyEntity(const Entity entity) {
    ecs_assert_entity_alive(entity);

    EntityRecord& record = this->getEntityRecord(entity);
    const TableId currentTid = record.tid;
    Table& currentTable = *this->tables_map.tables[currentTid];
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
    const Table& table = *this->getTables()[record.tid];

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
    const Table& table = *this->getTables()[record.tid];

    void* value = table.getComponent(record.row, cid);

    memcpy(value, component, componentRecord.size);
}


void EntityManager::removeComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");

    EntityRecord& record = this->getEntityRecord(entity);
    const ResolvedTableEdge resolved = this->resolveRemoveEdge(record.tid, cid);
    if (resolved.edge->tid == record.tid) {
        return;
    }

    if (resolved.edge->migration == nullptr) {
        const ComponentRecord& componentRecord = this->getComponentRecord(cid);
        resolved.edge->migration = this->compileMigration(
            *resolved.from, *resolved.to, componentRecord.onRemove, true);
    }

    resolved.edge->migration(
        resolved.from->columns,
        resolved.to->columns,
        this,
        resolved.from,
        resolved.to,
        &record,
        entity);
}

bool EntityManager::hasComponent(const Entity entity, const ComponentId cid) {
    ecs_assert_entity_alive(entity);
    ecs_assert(this->isComponentRegistered(cid), "component is not registered");
    const EntityRecord& record = this->getEntityRecord(entity);
    return this->tables_map.tables[record.tid]->hasComponent(cid);
}

std::pair<Table&, EntityRow> EntityManager::getTable(const Entity entity) {
    EntityRecord& record = this->getEntityRecord(entity);
    return {
        *this->getTables()[record.tid],
        record.row
    };
}
