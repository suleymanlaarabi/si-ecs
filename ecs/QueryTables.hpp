#pragma once

#include "ComponentRegistry.hpp"
#include "Query.hpp"
#include "TableRegistry.hpp"

inline const EcsVec<TableId>* querySeedTables(const Query& query, const ComponentRegistry& componentRegistry) {
    if (query.required_count == 0) {
        return nullptr;
    }

    const EcsVec<TableId>* seed = nullptr;

    for (uint8_t i = 0; i < query.required_count; ++i) {
        const auto& tables = componentRegistry.getComponentRecord(query.required[i]).tables;
        if (seed == nullptr || tables.size < seed->size) {
            seed = &tables;
        }
    }

    return seed;
}

template <typename Func>
inline void forEachMatchingTable(const Query& query, const ComponentRegistry& componentRegistry,
                                 TableRegistry& tableRegistry, Func&& func) {
    auto& tables = tableRegistry.getTables();

    if (const auto* seed = querySeedTables(query, componentRegistry); seed != nullptr) {
        for (const TableId tid : *seed) {
            if (Table& table = tables[tid]; query.matchTable(table)) {
                func(tid, table);
            }
        }
        return;
    }

    for (TableId tid = 0; tid < tables.size(); ++tid) {
        if (Table& table = tables[tid]; query.matchTable(table)) {
            func(tid, table);
        }
    }
}
