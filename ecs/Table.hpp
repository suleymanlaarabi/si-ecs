#pragma once
#include <sys/types.h>

#include "BloomFilter.hpp"
#include "EcsType.hpp"
#include "SparseSet.hpp"

struct Column {
    uint64_t size;
    void* data;

    [[nodiscard]] void* getRow(const EntityRow row) const {
        return static_cast<uint8_t*>(data) + row * size;
    }
};

class ComponentRegistry;

struct Table {
    EntityType type;
    SparseIndices indices;
    Entity* entities = nullptr;
    Column* columns = nullptr;
    uint32_t bucketIndex = 0; // used for TableMap.hpp
    uint64_t count = 0;
    uint64_t capacity = 0;

    void growIfNeeded();

public:
    BloomFilter bloom;
    explicit Table(EntityType type, ComponentRegistry& componentRegistry, TableId tid) noexcept;
    EntityRow addEntity(Entity entity);

    Entity removeEntity(EntityRow row);

    [[nodiscard]] void* getComponentByIndex(EntityRow row, uint16_t columnIndex) const;
    [[nodiscard]] void* getComponent(EntityRow row, ComponentId cid) const;
    [[nodiscard]] void* getColumn(ComponentId cid) const;
    [[nodiscard]] Entity* getEntities() const;
    [[nodiscard]] const EntityType& getType() const noexcept;
    [[nodiscard]] bool hasComponent(ComponentId cid) const;

    [[nodiscard]] uint size() const noexcept {
        return count;
    };
};
