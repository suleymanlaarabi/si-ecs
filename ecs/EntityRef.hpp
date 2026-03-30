#pragma once
#include "ComponentType.hpp"
#include "EcsType.hpp"

class World;

class EntityRef {
    World& world;
    Entity entity;

public:
    explicit EntityRef(World&, Entity);

    template <typename T>
    // ReSharper disable once CppMemberFunctionMayBeConst
    EntityRef add() {
        this->add_id(ComponentType::id<T>());
        return *this;
    }

    template <typename T>
    EntityRef set(T& value) {
        this->add_id(ComponentType::id<T>(), &value);
        return *this;
    }

private:
    void add_id(ComponentId) const;
    void set_id(ComponentId, const void* value) const;
};
