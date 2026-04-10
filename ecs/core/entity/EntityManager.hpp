#pragma once
#include <utility>
#include "ComponentRegistry.hpp"
#include "EcsType.hpp"
#include "EntityRegistry.hpp"
#include "JitCompiler.hpp"
#include "TableRegistry.hpp"

struct ResolvedTableEdge {
    Table* from;
    Table* to;
    TableEdge* edge;
};

class EntityManager : public EntityRegistry, public ComponentRegistry, public TableRegistry {
    void finalizeRowMigration(Table &from, EntityRecord &record, EntityRow newRow);
    [[nodiscard]] ResolvedTableEdge resolveAddEdge(TableId fromTid, ComponentId cid);
    [[nodiscard]] ResolvedTableEdge resolveRemoveEdge(TableId fromTid, ComponentId cid);

    [[nodiscard]] JitMigrationFn compileMigration(const Table& from, const Table& to);

    void migrateEntityRow(Table &from, Table &to, EntityRecord &record, Entity entity, JitMigrationFn migration);

public:
    std::pair<Table &, EntityRow> getTable(Entity entity);

    void addComponent(Entity entity, ComponentId cid);

    void destroyEntity(Entity entity);

    void *getComponent(Entity entity, ComponentId cid);

    void setComponent(Entity entity, ComponentId cid, const void *component);

    void removeComponent(Entity entity, ComponentId cid);

    bool hasComponent(Entity entity, ComponentId cid);
};
