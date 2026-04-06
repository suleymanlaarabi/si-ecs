#pragma once
#include "TypeCounter.hpp"

struct PluginRecord {};

class World;

template <typename T>
concept IsPlugin = requires(World& world) {
    { T::build(world) };
};

class PluginRegistry {
    int count = -1;
    using Counter = reflection::TypeCounter<PluginRegistry>;

public:
    template <typename T>
    void registerPlugin(World& world) {
        const uint16_t id = Counter::id<T>();
        if (count >= id) {
            return;
        }
        count = id;
        T::build(world);
    }
};
