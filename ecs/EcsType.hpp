#pragma once
#include <cstdint>

template <typename T>
consteval size_t ecs_sizeof() {
    if constexpr (requires { sizeof(T); }) {
        if constexpr (std::is_empty_v<T>) {
            return 0;
        } else {
            return sizeof(T);
        }
    } else {
        return 0;
    }
}

using TableId = uint16_t;
static constexpr TableId InvalidTableId = UINT16_MAX;
using ComponentId = uint16_t;
using EntityRow = uint32_t;
using EntityId = uint32_t;
using Generation = uint16_t;
using QueryId = uint16_t;

struct Entity {
    EntityId index;
    Generation generation;

    bool operator==(const Entity&) const = default;
};

struct EntityType {
    // already sorted
    ComponentId* cids = nullptr;
    uint16_t count = 0;

    void release() const;

    bool operator==(const EntityType& other) const noexcept;

    [[nodiscard]] uint16_t findIndex(ComponentId cid) const;

    [[nodiscard]] EntityType withAdd(ComponentId cid) const;
    [[nodiscard]] EntityType clone() const;

    void add(ComponentId cid);

    [[nodiscard]] EntityType withRemove(ComponentId cid) const;

    [[nodiscard]] ComponentId min() const;
    [[nodiscard]] ComponentId max() const;
};
