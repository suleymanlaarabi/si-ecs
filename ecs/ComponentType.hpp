#pragma once
#include <cstdint>
#include "TypeCounter.hpp"

template <typename Tag>
class TypeIdRegistry {
    template <typename>
    struct Family;

public:
    using Counter = reflection::TypeCounter<Family<Tag>>;

    template <typename T>
    static uint16_t id() {
        return Counter::template id<T>();
    }
};

class ComponentType : private TypeIdRegistry<ComponentType> {
public:
    using TypeIdRegistry<ComponentType>::id;
};
