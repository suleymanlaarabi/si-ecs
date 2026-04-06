#pragma once
#include "ComponentType.hpp"
#include "Query.hpp"
#include "Relations.hpp"
#include "SystemDesc.hpp"
#include "Table.hpp"
#include "WorldAccess.hpp"
#include "typing.hpp"
#include <tuple>
#include <utility>

template <typename T>
concept HasFromWorldTable = requires(World& world, const Table& table) {
    T::fromWorldTable(world, table);
};

template <typename T>
concept HasFromWorldRow =
    requires(World& world, const Table& table, const EntityRow row) {
        T::fromWorldRow(world, table, row);
    };

template <typename T>
concept HasQueryTerms = requires(Query& query) { T::addQueryTerms(query); };

template <typename Param>
struct FromWorldRowCache {
    World* world;
    const Table* table;
};

template <typename Param, typename OriginalParam>
struct IsSupportedSystemParam {
    static constexpr bool value =
        std::same_as<Param, Entity> || HasFromWorldTable<Param> ||
        HasFromWorldRow<Param> || std::is_lvalue_reference_v<OriginalParam>;
};

template <typename Param>
struct SystemParamTraits {
    static constexpr bool contributes_query = !std::same_as<Param, Entity> &&
        !HasFromWorldTable<Param> &&
        !HasFromWorldRow<Param>;

    __attribute__((always_inline)) static inline decltype(auto)
    resolveTable(World& world, const Table& table) {
        if constexpr (std::same_as<Param, Entity>) {
            return table.getEntities();
        } else if constexpr (HasFromWorldTable<Param>) {
            return Param::fromWorldTable(world, table);
        } else if constexpr (HasFromWorldRow<Param>) {
            return FromWorldRowCache<Param>{.world = &world, .table = &table};
        } else {
            return static_cast<Param*>(table.getColumn(ComponentType::id<Param>()));
        }
    }

    template <typename Cache>
    __attribute__((always_inline)) static inline decltype(auto)
    resolveRow(Cache& cache, const uint32_t row) {
        if constexpr (std::same_as<Param, Entity>) {
            return cache[row];
        }
        if constexpr (HasFromWorldTable<Param>) {
            return cache;
        } else if constexpr (HasFromWorldRow<Param>) {
            return Param::fromWorldRow(*cache.world, *cache.table, row);
        } else {
            return cache[row];
        }
    }
};

template <typename OriginalParamsTuple, typename ParamsTuple, size_t... I>
consteval bool validateSystemParams(std::index_sequence<I...>) {
    return (IsSupportedSystemParam<
            std::tuple_element_t<I, ParamsTuple>,
            std::tuple_element_t<I, OriginalParamsTuple>>::value &&
        ...);
}

template <typename Param>
static inline void appendSystemQueryParam(Query& query) {
    if constexpr (HasQueryTerms<Param>) {
        Param::addQueryTerms(query);
    } else if constexpr (SystemParamTraits<Param>::contributes_query) {
        applyTerm<With<Param>>(query);
    }
}

template <typename ParamsTuple, size_t... I>
static inline Query buildSystemQuery(Query query, std::index_sequence<I...>) {
    (appendSystemQueryParam<std::tuple_element_t<I, ParamsTuple>>(query), ...);
    return query;
}

template <typename ParamsTuple, typename Func, typename CacheTuple, size_t... I>
__attribute__((always_inline)) static inline void
callRow(Func&& fn, CacheTuple& caches, const uint32_t row,
        std::index_sequence<I...>) {
    fn(SystemParamTraits<std::tuple_element_t<I, ParamsTuple>>::resolveRow(
        std::get<I>(caches), row)...);
}

template <typename... Params>
struct ComponentsIter {
    template <typename Func>
    __attribute__((always_inline)) static inline void
    iter(Func&& func, const Table& table, World& world) {
        std::tuple<decltype(SystemParamTraits<Params>::resolveTable(world,
                                                                    table))...>
            caches{SystemParamTraits<Params>::resolveTable(world, table)...};

        for (uint32_t i = 0; i < table.size(); ++i) {
            callRow<std::tuple<Params...>>(func, caches, i,
                                           std::index_sequence_for<Params...>{});
        }
    }
};

template <auto func, typename... Terms>
SystemDesc each() {
    using Func = decltype(func);
    using OriginalArgs = function_args<Func>;
    using Args = function_args_raw<Func>;

    static_assert(
        validateSystemParams<OriginalArgs, Args>(tuple_index_sequence<Args>()),
        "Unsupported system param. Use T& or const T& for components, Entity for "
        "entity access, "
        "or implement fromWorldTable(World&, const Table&) / "
        "fromWorldRow(World&, const Table&, EntityRow).");

    return {
        .query = buildSystemQuery<Args>(query<Terms...>(),
                                        tuple_index_sequence<Args>()),
        .callback =
        +[](const EcsVec<TableId>& tids, World& world) {
            Table* tables = worldTables(world).data();

            for (const TableId tid : tids) {
                tuple_to_t<Args, ComponentsIter>::iter(func, tables[tid], world);
            }
        }
    };
}

template <auto func>
SystemDesc task() {
    using Func = decltype(func);
    using Args = function_args<Func>;
    return {
        .query = {},
        .callback = +[](const EcsVec<TableId>&, World& world) {
            if constexpr (std::is_same_v<std::tuple<std::remove_const<World&>>,
                                         Args>) {
                func(world);
            } else {
                func();
            }
        }
    };
}

template <typename Relation>
struct Related {
    Table& targetTable;
    EntityRow targetRow;

    static Related fromWorldRow(World& world, const Table& table,
                                const EntityRow row) {
        const Entity parent =
            static_cast<RelationTarget<Relation>*>(
                table.getColumn(ComponentType::id<RelationTarget<Relation>>()))[row]
            .target;

        auto [targetTable, targetRow] = worldGetTable(world, parent);

        return {targetTable, targetRow};
    }

    template <typename T>
    T& get() {
        return *static_cast<T*>(this->targetTable.getComponent(
            this->targetRow, ComponentType::id<T>()));
    }

    template <typename T>
    [[nodiscard]] bool has() const {
        return this->targetTable.hasComponent(ComponentType::id<T>());
    }

    static void addQueryTerms(Query& query) {
        applyTerm<With<RelationTarget<Relation>>>(query);
    }
};

using Parent = Related<ChildOf>;
