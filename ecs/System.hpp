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
concept HasFromTable = requires(World& world, const Table& table) {
    T::fromTable(world, table);
};

template <typename T>
concept HasFromWorld = requires(World& world) {
    T::fromWorld(world);
};

template <typename T>
struct has_from_world_trait : std::bool_constant<HasFromWorld<T>> {};

template <typename T>
concept HasFromRow =
    requires(World& world, const Table& table, const EntityRow row) {
        T::fromRow(world, table, row);
    };

template <typename T>
concept HasQueryTerms = requires(Query& query) { T::addQueryTerms(query); };

template <typename Param>
struct RowCache {
    World* world;
    const Table* table;
};

template <typename Param>
struct SystemColumn {
    Param* __restrict data;

    __attribute__((always_inline)) Param& operator[](const uint32_t row) const {
        return data[row];
    }
};

template <typename Param>
struct SystemColumn<const Param> {
    const Param* __restrict data;

    __attribute__((always_inline)) const Param& operator[](const uint32_t row) const {
        return data[row];
    }
};

template <typename Param>
inline constexpr bool contributes_query_v =
    !std::same_as<Param, Entity> &&
    !HasFromWorld<Param> &&
    !HasFromTable<Param> &&
    !HasFromRow<Param>;

template <typename Param, typename OriginalParam>
inline constexpr bool valid_system_param_v =
    std::same_as<Param, Entity> ||
    HasFromWorld<Param> ||
    HasFromTable<Param> ||
    HasFromRow<Param> ||
    std::is_lvalue_reference_v<OriginalParam>;

template <typename OriginalParam, typename Param>
inline constexpr bool fast_component_param_v =
    std::is_lvalue_reference_v<OriginalParam> &&
    !std::same_as<Param, Entity> &&
    !HasFromWorld<Param> &&
    !HasFromTable<Param> &&
    !HasFromRow<Param>;

template <typename Param>
inline constexpr bool fast_scalar_param_v =
    HasFromWorld<Param> || HasFromTable<Param>;

template <typename ParamsTuple, typename OriginalParamsTuple, size_t... I>
consteval bool validateSystemParams(std::index_sequence<I...>) {
    return (valid_system_param_v<
            std::tuple_element_t<I, ParamsTuple>,
            std::tuple_element_t<I, OriginalParamsTuple>> &&
        ...);
}

template <typename OriginalArgs, typename Args, size_t... I>
consteval bool supportsFastEach(std::index_sequence<I...>) {
    return ((fast_component_param_v<
                std::tuple_element_t<I, OriginalArgs>,
                std::tuple_element_t<I, Args>> ||
             fast_scalar_param_v<std::tuple_element_t<I, Args>>) &&
        ...);
}

template <typename Param>
static inline void appendSystemQueryParam(Query& query) {
    if constexpr (HasQueryTerms<Param>) {
        Param::addQueryTerms(query);
    } else if constexpr (contributes_query_v<Param>) {
        applyTerm<With<Param>>(query);
    }
}

template <typename ParamsTuple, size_t... I>
static inline Query buildSystemQuery(Query query, std::index_sequence<I...>) {
    (appendSystemQueryParam<std::tuple_element_t<I, ParamsTuple>>(query), ...);
    return query;
}

template <typename Param, typename FromWorldArgs>
__attribute__((always_inline)) static inline decltype(auto)
loadTable(World& world, FromWorldArgs& fromWorldArgs, const Table& table) {
    if constexpr (std::same_as<Param, Entity>) {
        return table.getEntities();
    } else if constexpr (HasFromWorld<Param>) {
        return std::get<Param>(fromWorldArgs);
    } else if constexpr (HasFromTable<Param>) {
        return Param::fromTable(world, table);
    } else if constexpr (HasFromRow<Param>) {
        return RowCache<Param>{.world = &world, .table = &table};
    } else {
        using Component = std::remove_const_t<Param>;
        return SystemColumn<Param>{
            .data = static_cast<Component*>(table.getColumn(ComponentType::id<Component>()))
        };
    }
}

template <typename Param, typename Cache>
__attribute__((always_inline)) static inline decltype(auto)
loadRow(Cache& cache, const uint32_t row) {
    if constexpr (std::same_as<Param, Entity>) {
        return cache[row];
    }
    if constexpr (HasFromWorld<Param> || HasFromTable<Param>) {
        return cache;
    } else if constexpr (HasFromRow<Param>) {
        return Param::fromRow(*cache.world, *cache.table, row);
    } else {
        return cache[row];
    }
}

template <typename A, typename B>
__attribute__((always_inline)) static inline void
assumeDistinct(const A&, const B&) {}

template <typename A, typename B>
__attribute__((always_inline)) static inline void
assumeDistinct(const SystemColumn<A>& a, const SystemColumn<B>& b) {
    if (__builtin_expect(static_cast<const void*>(a.data) == static_cast<const void*>(b.data), 0)) {
        __builtin_unreachable();
    }
}

template <typename First>
__attribute__((always_inline)) static inline void
assumeDistinctAll(First&) {}

template <typename First, typename Second, typename... Rest>
__attribute__((always_inline)) static inline void
assumeDistinctAll(First& first, Second& second, Rest&... rest) {
    assumeDistinct(first, second);
    (assumeDistinct(first, rest), ...);
    assumeDistinctAll(second, rest...);
}

template <auto func, typename... Params, typename... Caches>
__attribute__((always_inline)) static inline void
runRows(const uint32_t size, Caches&&... caches) {
    assumeDistinctAll(caches...);
    #pragma GCC ivdep
    for (uint32_t row = 0; row < size; ++row) {
        func(loadRow<Params>(caches, row)...);
    }
}

template <auto func, typename ParamsTuple, typename FromWorldArgs, size_t... I>
__attribute__((always_inline)) static inline void
runTable(World& world, FromWorldArgs& fromWorldArgs, const Table& table, std::index_sequence<I...>) {
    runRows<func, std::tuple_element_t<I, ParamsTuple>...>(
        table.size(),
        loadTable<std::tuple_element_t<I, ParamsTuple>>(world, fromWorldArgs, table)...);
}

template <typename T>
struct RestrictRef;

template <typename T>
struct RestrictRef<T&> {
    using type = T& __restrict__;
};

template <typename T>
struct RestrictRef<const T&> {
    using type = const T& __restrict__;
};

template <typename T>
using restrict_ref_t = RestrictRef<T>::type;

template <typename OriginalParam, typename Param>
struct FastInvokeArg {
    using type = OriginalParam;
};

template <typename OriginalParam, typename Param>
    requires fast_component_param_v<OriginalParam, Param>
struct FastInvokeArg<OriginalParam, Param> {
    using type = restrict_ref_t<OriginalParam>;
};

template <typename OriginalParam, typename Param>
using fast_invoke_arg_t = FastInvokeArg<OriginalParam, Param>::type;

template <typename OriginalParam, typename Param, typename FromWorldArgs>
__attribute__((always_inline)) static inline decltype(auto)
loadFastCache(World& world, FromWorldArgs& fromWorldArgs, const Table& table) {
    if constexpr (fast_component_param_v<OriginalParam, Param>) {
        using Component = std::remove_const_t<Param>;
        return static_cast<Param* __restrict>(
            static_cast<Component*>(table.getColumn(ComponentType::id<Component>())));
    } else if constexpr (HasFromWorld<Param>) {
        return std::get<Param>(fromWorldArgs);
    } else {
        return Param::fromTable(world, table);
    }
}

template <typename OriginalParam, typename Param, typename Cache>
__attribute__((always_inline)) static inline decltype(auto)
loadFastArg(Cache& cache, const uint32_t row) {
    if constexpr (fast_component_param_v<OriginalParam, Param>) {
        return cache[row];
    } else {
        return cache;
    }
}

template <auto func, typename... InvokeArgs>
__attribute__((always_inline)) static inline void
invokeFast(InvokeArgs... args) {
    func(args...);
}

template <auto func, typename OriginalArgs, typename Args, typename... Caches, size_t... I>
__attribute__((always_inline)) static inline void
runFastRows(const uint32_t size, std::index_sequence<I...>, Caches... caches) {
    #pragma GCC ivdep
    for (uint32_t row = 0; row < size; ++row) {
        invokeFast<func, fast_invoke_arg_t<
            std::tuple_element_t<I, OriginalArgs>,
            std::tuple_element_t<I, Args>>...>(
            loadFastArg<
                std::tuple_element_t<I, OriginalArgs>,
                std::tuple_element_t<I, Args>>(caches, row)...);
    }
}

template <auto func, typename OriginalArgs, typename Args, typename FromWorldArgs, size_t... I>
__attribute__((always_inline)) static inline void
runFastTable(World& world, FromWorldArgs& fromWorldArgs, const Table& table, std::index_sequence<I...>) {
    runFastRows<func, OriginalArgs, Args>(
        table.size(),
        std::index_sequence<I...>{},
        loadFastCache<
            std::tuple_element_t<I, OriginalArgs>,
            std::tuple_element_t<I, Args>>(world, fromWorldArgs, table)...);
}

template <typename Tuple, size_t... I>
Tuple fetchFromWorlds(World& world, std::index_sequence<I...>) {
    return {std::tuple_element_t<I, Tuple>::fromWorld(world)...};
}

template <typename Param, typename OriginalParam>
inline constexpr bool valid_task_param_v =
    std::is_same_v<OriginalParam, World&> ||
    std::is_same_v<OriginalParam, const World&> ||
    HasFromWorld<Param>;

template <typename ParamsTuple, typename OriginalParamsTuple, size_t... I>
consteval bool validateTaskParams(std::index_sequence<I...>) {
    return (valid_task_param_v<
            std::tuple_element_t<I, ParamsTuple>,
            std::tuple_element_t<I, OriginalParamsTuple>> &&
        ...);
}

template <typename Param>
struct task_from_world_trait : std::bool_constant<HasFromWorld<Param>> {};

template <typename OriginalParam, typename Param, typename FromWorldArgs>
__attribute__((always_inline)) static inline decltype(auto)
loadTaskArg(World& world, FromWorldArgs& fromWorldArgs) {
    if constexpr (std::is_same_v<OriginalParam, World&> ||
                  std::is_same_v<OriginalParam, const World&>) {
        return world;
    } else {
        return std::get<Param>(fromWorldArgs);
    }
}

template <auto func, typename OriginalArgs, typename Args, typename FromWorldArgs, size_t... I>
__attribute__((always_inline)) static inline void
runTask(World& world, FromWorldArgs& fromWorldArgs, std::index_sequence<I...>) {
    func(loadTaskArg<
            std::tuple_element_t<I, OriginalArgs>,
            std::tuple_element_t<I, Args>>(world, fromWorldArgs)...);
}

template <auto func, typename... Terms>
SystemDesc each() {
    using Func = decltype(func);
    using OriginalArgs = function_args<Func>;
    using Args = function_args_raw<Func>;
    using FromWorldsArgs = filter_tuple_t<Args, has_from_world_trait>;
    static_assert(
        validateSystemParams<Args, OriginalArgs>(tuple_index_sequence<Args>()),
        "Unsupported system param. Use T& or const T& for components, Entity for "
        "entity access, "
        "or implement fromTable(World&, const Table&) / "
        "fromRow(World&, const Table&, EntityRow).");

    return {
        .query = buildSystemQuery<Args>(query<Terms...>(), tuple_index_sequence<Args>()),
        .callback =
        +[](const EcsVec<TableId>& tids, World& world) {
            Table* tables = worldTables(world).data();
            FromWorldsArgs fromWorldArgs =
                fetchFromWorlds<FromWorldsArgs>(world, tuple_index_sequence<FromWorldsArgs>());

            for (const TableId tid : tids) {
                if constexpr (supportsFastEach<OriginalArgs, Args>(tuple_index_sequence<Args>())) {
                    runFastTable<func, OriginalArgs, Args>(
                        world, fromWorldArgs, tables[tid], tuple_index_sequence<Args>());
                } else {
                    runTable<func, Args>(world, fromWorldArgs, tables[tid], tuple_index_sequence<Args>());
                }
            }
        }
    };
}

template <auto func>
SystemDesc task() {
    using Func = decltype(func);
    using OriginalArgs = function_args<Func>;
    using Args = function_args_raw<Func>;
    using FromWorldsArgs = filter_tuple_t<Args, task_from_world_trait>;
    static_assert(
        validateTaskParams<Args, OriginalArgs>(tuple_index_sequence<Args>()),
        "Unsupported task param. Use World& / const World& or implement fromWorld(World&).");

    return {
        .query = {},
        .callback = +[](const EcsVec<TableId>&, World& world) {
            FromWorldsArgs fromWorldArgs =
                fetchFromWorlds<FromWorldsArgs>(world, tuple_index_sequence<FromWorldsArgs>());
            runTask<func, OriginalArgs, Args>(world, fromWorldArgs, tuple_index_sequence<Args>());
        }
    };
}

template <typename Relation>
struct Related {
    Table& targetTable;
    EntityRow targetRow;

    static Related fromRow(World& world, const Table& table,
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
