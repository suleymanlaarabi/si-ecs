#pragma once
#include <utility>

#include "Allocator.hpp"
#include "ComponentRegistry.hpp"
#include "EntityRegistry.hpp"
#include "TableRegistry.hpp"


class EntityManager : public EntityRegistry, public ComponentRegistry, public TableRegistry {
    TempAllocator tmpAllocator;
    void finalizeRowMigration(Table& from, EntityRecord& record, Entity entity, EntityRow newRow);
    void migrateEntityRowAdd(Table& from, Table& to, EntityRecord& record, Entity entity,
                             uint16_t insertIndex);
    void migrateEntityRowRemove(Table& from, Table& to, EntityRecord& record, const Entity entity,
                                const uint16_t removeIndex);
    void migrateEntityRow(Table& from, Table& to, EntityRecord& record, Entity entity);

public:
    std::pair<Table&, EntityRow> getTable(Entity entity);
    void addComponent(Entity entity, ComponentId cid);
    void addBatchComponents(Entity entity, const ComponentId* cids, uint32_t cidCount);
    void destroyEntity(Entity entity);
    void* getComponent(Entity entity, ComponentId cid);
    void setComponent(Entity entity, ComponentId cid, const void* component);
    void removeComponent(Entity entity, ComponentId cid);
    bool hasComponent(Entity entity, ComponentId cid);
};
