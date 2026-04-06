#pragma once
#include <functional>
#include "EcsVec.hpp"
#include "Query.hpp"

class World;

struct SystemCondition {
    void* ctx = nullptr;

    bool (*check)(void* ctx, World&) = nullptr;
};

template <typename T>
concept IntoSystemCondition = requires(World& world) {
    { T::condition(world) } -> std::same_as<SystemCondition>;
};

struct SystemDesc {
    Query query;
    std::function<void(const EcsVec<TableId>& tables, World&)> callback;
    std::vector<SystemCondition> conditions;

    template <IntoSystemCondition Condition>
    SystemDesc condition(World& world) {
        this->conditions.push_back(Condition::condition(world));

        return std::move(*this);
    }
};
