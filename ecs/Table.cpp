#include "Table.hpp"
#include <cstdlib>
#include <cstring>
#include "ComponentRegistry.hpp"

Table::Table(const EntityType type, ComponentRegistry& componentRegistry,
             const TableId tid) noexcept : indices(
    type.min(), type.max()) {
    this->type = type;
    this->columns = static_cast<Column*>(malloc(sizeof(Column) * this->type.count));
    this->capacity = 1;
    this->entities = static_cast<Entity*>(malloc(sizeof(Entity)));

    this->bloom = bloomFromEntityType(type);
    for (uint i = 0; i < this->type.count; i++) {
        ComponentRecord& componentRecord = componentRegistry.getComponentRecord(this->type.cids[i]);
        componentRecord.tables.push_back(tid);
        this->indices.set(type.cids[i], i);

        const size_t size = componentRecord.size;
        if (size != 0) {
            this->columns[i].data = malloc(size);
        }
        this->columns[i].size = size;
    }
}

void Table::growIfNeeded() {
    if (count < capacity) {
        return;
    }

    const uint64_t newCapacity = (capacity == 0) ? 1 : capacity * 2;

    auto* newEntities = static_cast<Entity*>(
        realloc(entities, newCapacity * sizeof(Entity))
    );

    entities = newEntities;

    for (uint64_t i = 0; i < this->type.count; ++i) {
        auto& [size, data] = columns[i];
        if (size == 0) {
            continue;
        }
        const uint64_t oldBytes = capacity * size;
        const uint64_t newBytes = newCapacity * size;

        data = std::realloc(data, newBytes);

        if (newBytes > oldBytes) {
            memset(static_cast<uint8_t*>(data) + oldBytes, 0, newBytes - oldBytes);
        }
    }

    capacity = newCapacity;
}


EntityRow Table::addEntity(const Entity entity) {
    growIfNeeded();

    entities[count] = entity;

    count += 1;

    return count - 1;
}

Entity Table::removeEntity(const EntityRow row) {
    Entity swapped(0, 0);

    if (count == 0) {
        return swapped;
    }

    if (const EntityRow last = count - 1; row != last) {
        swapped = entities[last];
        entities[row] = entities[last];

        for (uint64_t i = 0; i < this->type.count; ++i) {
            Column& col = columns[i];
            std::memcpy(col.getRow(row), col.getRow(last), col.size);
        }
    }

    count -= 1;
    return swapped;
}

void* Table::getComponentByIndex(const EntityRow row, const uint16_t columnIndex) const {
    return columns[columnIndex].getRow(row);
}

void* Table::getColumn(const ComponentId cid) const {
    return this->columns[this->indices.at(cid)].data;
}

void* Table::getComponent(const EntityRow row, const ComponentId cid) const {
    return this->getComponentByIndex(row, this->indices.at(cid));
}

[[nodiscard]] Entity* Table::getEntities() const {
    return this->entities;
}

[[nodiscard]] const EntityType& Table::getType() const noexcept {
    return this->type;
}

[[nodiscard]] bool Table::hasComponent(const ComponentId cid) const {
    return this->indices.has(cid);
}
