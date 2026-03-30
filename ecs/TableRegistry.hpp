#pragma once
#include <functional>
#include <vector>
#include "Table.hpp"
#include "TableMap.hpp"

struct EntityTypeHasher {
    size_t operator()(const EntityType& e) const noexcept;
};

class TableRegistry {
protected:
    TableMap tables_map;

public:
    std::vector<std::function<void(Table&, TableId tid)>> onTableCreated;
    TableRegistry();

    std::pair<TableId, Table&> findOrCreateTable(EntityType type, ComponentRegistry& componentRegistry) noexcept;
    std::pair<TableId, Table&> findOrCreateTable(EntityType type, ComponentRegistry& componentRegistry,
                                                 bool& isCreated) noexcept;
    std::vector<Table>& getTables() noexcept;
};
