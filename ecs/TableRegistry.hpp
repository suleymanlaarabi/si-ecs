#pragma once
#include <functional>
#include <vector>
#include "Table.hpp"
#include "TableMap.hpp"

class TableRegistry {
protected:
    TableMap tables_map;

public:
    std::vector<std::function<void(Table&, TableId tid)>> onTableCreated;
    TableRegistry();

    std::pair<TableId, Table&> findOrCreateTable(EntityType&& type, ComponentRegistry& componentRegistry) noexcept;
    std::vector<Table>& getTables() noexcept;
};
