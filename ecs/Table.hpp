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

struct TableEdges {
    SparseIndices add;
    SparseIndices remove;
};

struct Table {
    EntityType type;
    Entity* entities = nullptr;
    Column* columns = nullptr;
    TableEdges* edges = nullptr;
    uint32_t bucketIndex = 0; // used for TableMap.hpp
    uint32_t count = 0;
    uint32_t capacity = 0;
    BloomFilter bloom;

    explicit Table(EntityType&& type, ComponentRegistry& componentRegistry, TableId tid) noexcept;
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&& other) noexcept;
    Table& operator=(Table&& other) noexcept;
    ~Table();
    EntityRow addEntity(Entity entity);

    Entity removeEntity(EntityRow row);

    [[nodiscard]] void* getComponentByIndex(EntityRow row, uint16_t columnIndex) const;
    [[nodiscard]] void* getComponent(EntityRow row, ComponentId cid) const;
    [[nodiscard]] void* getColumn(ComponentId cid) const;
    [[nodiscard]] Entity* getEntities() const;
    [[nodiscard]] const EntityType& getType() const noexcept;
    [[nodiscard]] bool hasComponent(ComponentId cid) const;
    [[nodiscard]] bool hasAddEdge(ComponentId cid) const;
    [[nodiscard]] TableId getAddEdge(ComponentId cid) const;
    void setAddEdge(ComponentId cid, TableId tid);
    [[nodiscard]] bool hasRemoveEdge(ComponentId cid) const;
    [[nodiscard]] TableId getRemoveEdge(ComponentId cid) const;
    void setRemoveEdge(ComponentId cid, TableId tid);
    void resetEdges() const;

    [[nodiscard]] uint size() const noexcept {
        return count;
    };

private:
    void growIfNeeded();
};
