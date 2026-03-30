#pragma once
#include <vector>
#include "EcsAssert.hpp"
#include "EcsType.hpp"

struct EntityRecord {
    TableId tid = 0;
    Generation generation = 0;
    EntityRow row = 0;
    uint16_t listenersId = UINT16_MAX;
};

class EntityRegistry {
    std::vector<EntityRecord> entities = {{0, 0, 0}};
    std::vector<EntityId> availableEntities;

public:
    EntityRegistry() {
        this->entities.reserve(100000);
    }

    __attribute__((always_inline)) Entity createEntity() {
        if (this->availableEntities.empty()) {
            ecs_assert(this->entities.size() <= UINT32_MAX, "entity id limit reached");
            this->entities.push_back({0, 0, 0});
            return Entity{.index = static_cast<EntityId>(this->entities.size() - 1), .generation = 0};
        }
        const EntityId eid = this->availableEntities.back();
        this->availableEntities.pop_back();
        this->entities[eid].tid = 0;
        this->entities[eid].row = 0;
        return Entity{.index = eid, .generation = this->entities[eid].generation};
    }

    __attribute__((always_inline)) void destroyEntity(const Entity entity) {
        this->entities[entity.index].generation += 1;
        this->availableEntities.push_back(entity.index);
    }

    __attribute__((always_inline)) [[nodiscard]] bool isAlive(const Entity entity) const {
        return this->entities[entity.index].generation == entity.generation;
    }

    __attribute__((always_inline)) EntityRecord& getEntityRecord(const Entity entity) {
        return this->entities[entity.index];
    }
};
