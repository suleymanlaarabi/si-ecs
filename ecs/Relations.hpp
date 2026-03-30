#pragma once

#include "EcsVec.hpp"

class World;

struct ChildOf {
    static constexpr bool recursive = true;
};

template <typename>
struct RelationSource {
    EcsVec<Entity> entities;
};

template <typename T>
struct RelationTarget {
    Entity target;

    explicit RelationTarget(Entity entity) : target(entity) {}

    static void onSet(Entity entity, const RelationTarget<T>& target, World& world);
    static void onRemove(Entity entity, World& world);
};
