#pragma once
#include <cmath>

#include "World.hpp"

struct Singleton {
    template <typename Self>
    static auto& fromWorld(Self&, World& world) {
        using T = std::remove_cvref_t<Self>;
        return world.template getSingleton<T>();
    }
};

typedef float delta_t;

struct DeltaTime {
    float delta;

    static DeltaTime fromWorld(const World& world) {
        return {world.getDeltaTime()};
    }
};

template <float value>
class Interval {
public:
    static SystemCondition condition(World&) {
        static_assert(sizeof(Timer) == 8, "timer need to size 64 bits because is casted in void* for cache friendly");
        return {
            .ctx = std::bit_cast<void*>(Timer(value)),
            .check = +[](void* ctx, const World& world) {
                return std::bit_cast<Timer>(ctx).tick(world.getDeltaTime());
            }
        };
    }
};

template <auto Value>
class InState {
    using T = decltype(Value);

public:
    static SystemCondition condition(World& world) {
        return {
            .ctx = static_cast<void*>(&world.getSingleton<T>()),
            .check = +[](void* ctx, World&) {
                return *static_cast<T*>(ctx) == Value;
            }
        };
    };
};
