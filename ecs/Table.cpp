#include "Table.hpp"
#include <cstdlib>
#include <cstring>
#include "ComponentRegistry.hpp"

namespace {
    void releaseTableStorage(Table& table) {
        table.type.release();
        delete table.edges;
        if (table.columns != nullptr) {
            for (uint64_t i = 0; i < table.type.count; ++i) {
                free(table.columns[i].data);
            }
        }
        free(table.columns);
        free(table.entities);
        table.type = {};
        table.entities = nullptr;
        table.columns = nullptr;
        table.edges = nullptr;
        table.bucketIndex = 0;
        table.count = 0;
        table.capacity = 0;
        table.bloom = 0;
    }
}

Table::Table(EntityType&& type, ComponentRegistry& componentRegistry,
             const TableId tid) noexcept {
    this->type = type;
    type = {};
    this->columns = static_cast<Column*>(malloc(sizeof(Column) * this->type.count));
    this->capacity = 1;
    this->entities = static_cast<Entity*>(malloc(sizeof(Entity)));

    this->bloom = bloomFromEntityType(this->type);
    for (uint i = 0; i < this->type.count; i++) {
        ComponentRecord& componentRecord = componentRegistry.getComponentRecord(this->type.cids[i]);
        componentRecord.tables.push_back(tid);

        const size_t size = componentRecord.size;
        this->columns[i].data = nullptr;
        if (size != 0) {
            this->columns[i].data = malloc(size);
        }
        this->columns[i].size = size;
    }
}

Table::Table(Table&& other) noexcept : type(other.type),
                                       entities(other.entities),
                                       columns(other.columns),
                                       edges(other.edges),
                                       bucketIndex(other.bucketIndex),
                                       count(other.count),
                                       capacity(other.capacity),
                                       bloom(other.bloom) {
    other.type = {};
    other.entities = nullptr;
    other.columns = nullptr;
    other.edges = nullptr;
    other.bucketIndex = 0;
    other.count = 0;
    other.capacity = 0;
    other.bloom = 0;
}

Table& Table::operator=(Table&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    releaseTableStorage(*this);
    this->type = other.type;
    this->entities = other.entities;
    this->columns = other.columns;
    this->edges = other.edges;
    this->bucketIndex = other.bucketIndex;
    this->count = other.count;
    this->capacity = other.capacity;
    this->bloom = other.bloom;

    other.type = {};
    other.entities = nullptr;
    other.columns = nullptr;
    other.edges = nullptr;
    other.bucketIndex = 0;
    other.count = 0;
    other.capacity = 0;
    other.bloom = 0;
    return *this;
}

Table::~Table() {
    releaseTableStorage(*this);
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
    return this->columns[this->type.findIndex(cid)].data;
}

void* Table::getComponent(const EntityRow row, const ComponentId cid) const {
    return this->getComponentByIndex(row, this->type.findIndex(cid));
}

[[nodiscard]] Entity* Table::getEntities() const {
    return this->entities;
}

[[nodiscard]] const EntityType& Table::getType() const noexcept {
    return this->type;
}

[[nodiscard]] bool Table::hasComponent(const ComponentId cid) const {
    return this->type.findIndex(cid) != UINT16_MAX;
}

[[nodiscard]] bool Table::hasAddEdge(const ComponentId cid) const {
    return this->edges != nullptr && this->edges->add.has(cid);
}

[[nodiscard]] TableId Table::getAddEdge(const ComponentId cid) const {
    return this->edges->add.at(cid);
}

void Table::setAddEdge(const ComponentId cid, const TableId tid) {
    if (this->edges == nullptr) {
        this->edges = new TableEdges();
    }
    this->edges->add.set(cid, tid);
}

[[nodiscard]] bool Table::hasRemoveEdge(const ComponentId cid) const {
    return this->edges != nullptr && this->edges->remove.has(cid);
}

[[nodiscard]] TableId Table::getRemoveEdge(const ComponentId cid) const {
    return this->edges->remove.at(cid);
}

void Table::setRemoveEdge(const ComponentId cid, const TableId tid) {
    if (this->edges == nullptr) {
        this->edges = new TableEdges();
    }
    this->edges->remove.set(cid, tid);
}

void Table::resetEdges() const {
    if (this->edges == nullptr) {
        return;
    }
    this->edges->add.reset();
    this->edges->remove.reset();
}
