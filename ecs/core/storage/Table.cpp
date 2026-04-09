#include "Table.hpp"
#include <cstdlib>
#include <cstring>
#include "ComponentRegistry.hpp"

Table::Table(EntityType &&type, ComponentRegistry &componentRegistry,
             const TableId tid) noexcept {
    this->type = type;
    type = {};
    this->columns = static_cast<Column *>(malloc(sizeof(Column) * this->type.count));
    this->capacity = 1;
    this->entities = static_cast<Entity *>(malloc(sizeof(Entity)));

    this->bloom = bloomFromEntityType(this->type);
    for (uint i = 0; i < this->type.count; i++) {
        const ComponentId cid = this->type.cids[i];
        ComponentRecord &componentRecord = componentRegistry.getComponentRecord(cid);
        componentRecord.tables.push_back(tid);
        this->setAddEdge(cid, tid);
        const size_t size = componentRecord.size;
        this->columns[i].data = nullptr;
        if (size != 0) {
            this->columns[i].data = malloc(size);
        }
        this->columns[i].size = size;
    }
}

Table::Table(Table &&other) noexcept : type(other.type),
                                       entities(other.entities),
                                       columns(other.columns),
                                       edges(std::move(other.edges)),
                                       count(other.count),
                                       capacity(other.capacity),
                                       bloom(other.bloom) {
    other.type = {};
    other.entities = nullptr;
    other.columns = nullptr;
    other.edges.add.reset();
    other.edges.remove.reset();
    other.count = 0;
    other.capacity = 0;
    other.bloom = 0;
}

void Table::growIfNeeded() {
    if (count < capacity) {
        return;
    }

    const uint64_t newCapacity = (capacity == 0) ? 1 : capacity * 2;

    auto *newEntities = static_cast<Entity *>(
        realloc(entities, newCapacity * sizeof(Entity))
    );

    entities = newEntities;

    for (uint64_t i = 0; i < this->type.count; ++i) {
        auto &[size, data] = columns[i];
        if (size == 0) {
            continue;
        }
        const uint64_t oldBytes = capacity * size;
        const uint64_t newBytes = newCapacity * size;

        data = std::realloc(data, newBytes);

        if (newBytes > oldBytes) {
            memset(static_cast<uint8_t *>(data) + oldBytes, 0, newBytes - oldBytes);
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
            Column &col = columns[i];
            if (col.size == 0) {
                continue;
            }
            std::memcpy(col.getRow(row), col.getRow(last), col.size);
        }
    }

    count -= 1;
    return swapped;
}

void *Table::getComponentByIndex(const EntityRow row, const uint16_t columnIndex) const {
    return columns[columnIndex].getRow(row);
}

void *Table::getColumn(const ComponentId cid) const {
    return this->columns[this->type.findIndex(cid)].data;
}

void *Table::getComponent(const EntityRow row, const ComponentId cid) const {
    return this->getComponentByIndex(row, this->type.findIndex(cid));
}

[[nodiscard]] Entity *Table::getEntities() const {
    return this->entities;
}

[[nodiscard]] const EntityType &Table::getType() const noexcept {
    return this->type;
}

[[nodiscard]] bool Table::hasComponent(const ComponentId cid) const {
    return this->type.findIndex(cid) != UINT16_MAX;
}

[[nodiscard]] bool Table::hasComponent(const ComponentId cid, const TableId tid) const {
    return this->edges.add.atOrInvalid(cid) == tid;
}

[[nodiscard]] bool Table::hasAddEdge(const ComponentId cid) const {
    return this->edges.add.has(cid);
}

[[nodiscard]] TableId Table::getAddEdge(const ComponentId cid) const {
    return this->edges.add.at(cid);
}

void Table::setAddEdge(const ComponentId cid, const TableId tid) {
    this->edges.add.set(cid, tid);
}

[[nodiscard]] bool Table::hasRemoveEdge(const ComponentId cid) const {
    return this->edges.remove.has(cid);
}

[[nodiscard]] TableId Table::getRemoveEdge(const ComponentId cid) const {
    return this->edges.remove.at(cid);
}

void Table::setRemoveEdge(const ComponentId cid, const TableId tid) {
    this->edges.remove.set(cid, tid);
}
