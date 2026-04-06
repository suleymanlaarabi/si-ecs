#pragma once
#include "ComponentRegistry.hpp"
#include "EcsAssert.hpp"
#include "EcsType.hpp"
#include "EcsVec.hpp"

class Table;

#define MAX_QUERY_TERM 16

struct Query {
    uint64_t bloom = 0;
    ComponentId required[MAX_QUERY_TERM] = {UINT16_MAX};
    uint8_t required_count = 0;

    ComponentId excluded[MAX_QUERY_TERM] = {UINT16_MAX};
    uint8_t excluded_count = 0;

    [[nodiscard]] bool matchTable(const Table& table) const;

    Query& with(const ComponentId cid) {
        append(this->required, this->required_count, cid);
        this->bloom |= (1ull << (cid % 64));
        return *this;
    }

    Query& without(const ComponentId cid) {
        append(this->excluded, this->excluded_count, cid);
        return *this;
    }

    template <typename T>
    Query& with() {
        return this->with(ComponentRegistry::id<T>());
    }

    template <typename T>
    Query& without() {
        return this->without(ComponentRegistry::id<T>());
    }

private:
    static void append(ComponentId* terms, uint8_t& count, ComponentId cid) {
        for (uint8_t i = 0; i < count; ++i) {
            if (terms[i] == cid) {
                return;
            }
        }

        if (count >= MAX_QUERY_TERM) {
            ecs_assert(false, "query term limit reached");
        }

        terms[count++] = cid;
    }
};

template <typename T>
concept HasTermWith = requires() {
    T::required();
    T::count;
};

template <typename T>
concept HasTermWithout = requires() {
    T::excluded();
    T::count;
};

template <typename Term>
void applyTerm(Query& query) {
    if constexpr (HasTermWith<Term>) {
        const auto required = Term::required();
        for (uint8_t i = 0; i < Term::count; ++i) {
            query.with(required[i]);
        }
    }
    if constexpr (HasTermWithout<Term>) {
        const auto excluded = Term::excluded();
        for (uint8_t i = 0; i < Term::count; ++i) {
            query.without(excluded[i]);
        }
    }
}

template <typename... Components>
struct With {
    static std::array<ComponentId, MAX_QUERY_TERM> required() {
        return {ComponentRegistry::id<Components>()...};
    }

    static const uint8_t count = sizeof...(Components);
};

template <typename... Components>
struct Without {
    static std::array<ComponentId, MAX_QUERY_TERM> excluded() {
        return {ComponentRegistry::id<Components>()...};
    }

    static const uint8_t count = sizeof...(Components);
};

template <typename... Terms>
Query query() {
    Query query;
    (applyTerm<Terms>(query), ...);
    return query;
}

struct QueryCache {
    Query query;
    EcsVec<TableId> tables;

    ~QueryCache() {
        this->tables.free();
    }
};
