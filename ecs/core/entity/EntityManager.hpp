#pragma once
#include <utility>
#include "Allocator.hpp"
#include "ComponentRegistry.hpp"
#include "EcsType.hpp"
#include "EntityRegistry.hpp"
#include "JitCompiler.hpp"
#include "TableRegistry.hpp"

using MigrateEntityFn = void(*)(Entity entity, TableId tid, ComponentId cid);

class EntityManager : public EntityRegistry, public ComponentRegistry, public TableRegistry {
    TempAllocator tmpAllocator;
    JitCompiler compiler;

    void finalizeRowMigration(Table &from, EntityRecord &record, EntityRow newRow);

    void migrateEntityRow(Table &from, Table &to, EntityRecord &record, Entity entity);

public:
    std::pair<Table &, EntityRow> getTable(Entity entity);

    void addComponent(Entity entity, ComponentId cid);

    void destroyEntity(Entity entity);

    void *getComponent(Entity entity, ComponentId cid);

    void setComponent(Entity entity, ComponentId cid, const void *component);

    void removeComponent(Entity entity, ComponentId cid);

    bool hasComponent(Entity entity, ComponentId cid);
};
