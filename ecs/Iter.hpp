#pragma once
#include <functional>
#include <utility>

#include "Query.hpp"

class Table;
class World;

class RowView {
    World* world = nullptr;
    const Table* table = nullptr;
    EntityRow row = 0;

public:
    RowView() = default;
    RowView(World& world, const Table& table, EntityRow row);

    [[nodiscard]] Entity entity() const;
    [[nodiscard]] bool has(ComponentId cid) const;
    [[nodiscard]] void* get(ComponentId cid) const;

    template <typename T>
    [[nodiscard]] bool has() const {
        return this->has(ComponentType::id<T>());
    }

    template <typename T>
    T& get() const {
        return *static_cast<T*>(this->get(ComponentType::id<T>()));
    }

    template <typename T>
    T* tryGet() const {
        if (!this->has<T>()) {
            return nullptr;
        }
        return &this->get<T>();
    }
};

class RuntimeQuery {
    World* world = nullptr;
    Query desc;

public:
    RuntimeQuery() = default;
    RuntimeQuery(World& world, Query query = {});

    RuntimeQuery& with(ComponentId cid);
    RuntimeQuery& without(ComponentId cid);

    template <typename T>
    RuntimeQuery& with() {
        this->desc.with<T>();
        return *this;
    }

    template <typename T>
    RuntimeQuery& without() {
        this->desc.without<T>();
        return *this;
    }

    [[nodiscard]] const Query& query() const {
        return this->desc;
    }

    template <typename Func>
    void each(Func&& func) const {
        this->eachRow([&func](const RowView row) {
            func(row);
        });
    }

    void eachRow(const std::function<void(RowView)>& func) const;
};
