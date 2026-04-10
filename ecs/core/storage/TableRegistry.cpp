#include "TableRegistry.hpp"

#include "ComponentRegistry.hpp"

TableRegistry::TableRegistry() {
    ComponentRegistry componentRegistry;
    EntityType type{};
    bool isCreated;
    (void)this->tables_map.findOrCreate(std::move(type), componentRegistry, isCreated);
}

std::pair<TableId, Table&> TableRegistry::findOrCreateTable(EntityType&& type,
                                                            ComponentRegistry& componentRegistry) noexcept {
    bool isCreated = false;
    auto [tid, table] = this->tables_map.findOrCreate(std::move(type), componentRegistry, isCreated);

    if (isCreated) {
        for (const auto& hook : this->onTableCreated) {
            hook(table, tid);
        }
    }

    return {tid, table};
}

std::vector<Table*>& TableRegistry::getTables() noexcept {
    return this->tables_map.tables;
}
