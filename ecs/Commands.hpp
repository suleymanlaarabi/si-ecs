#pragma once
#include "Allocator.hpp"
#include "ComponentType.hpp"
#include "EcsType.hpp"
#include "EcsVec.hpp"

struct CommandAdd {
    Entity entity;
    ComponentId cid;
};

struct CommandRemove {
    Entity entity;
    ComponentId cid;
};

struct CommandSet {
    Entity entity;
    ComponentId cid;
    void* value;
};

class World;
class Table;

class Commands {
    World& world;
    TempAllocator buffer;
    EcsVec<CommandAdd> adds;
    EcsVec<CommandRemove> removes;
    EcsVec<CommandSet> sets;
    EcsVec<Entity> kills;

public:
    explicit Commands(World&);
    ~Commands();

    static Commands& fromTable(World&, const Table&);

    [[nodiscard]] Entity spawn() const;
    void kill(Entity);

    template <typename T>
    void add(const Entity entity) {
        this->add(entity, ComponentType::id<T>());
    }

    template <typename T>
    void remove(const Entity entity) {
        this->remove(entity, ComponentType::id<T>());
    }

    template <typename T>
    void set(const Entity entity, const T& value) {
        this->set(entity, ComponentType::id<T>(), &value, ecs_sizeof<T>());
    }

    void add(Entity entity, ComponentId cid);
    void remove(Entity entity, ComponentId cid);
    void set(Entity entity, ComponentId cid, const void* value, size_t size);

    void flush();
};
