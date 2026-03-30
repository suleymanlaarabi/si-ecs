#include "TableRegistry.hpp"

#include "ComponentRegistry.hpp"

TableRegistry::TableRegistry() {
    ComponentRegistry componentRegistry;
    constexpr EntityType type;
    bool isCreated;
    (void)this->tables_map.findOrCreate(type, componentRegistry, isCreated);
}

size_t EntityTypeHasher::operator()(const EntityType& e) const noexcept {
    uint64_t h = 0x9E3779B97F4A7C15ull ^ static_cast<uint64_t>(e.count);

    for (uint16_t i = 0; i < e.count; ++i) {
        h ^= static_cast<uint64_t>(e.cids[i]) + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
    }

    h ^= h >> 30;
    h *= 0xBF58476D1CE4E5B9ull;
    h ^= h >> 27;
    h *= 0x94D049BB133111EBull;
    h ^= h >> 31;
    return h;
}

std::pair<TableId, Table&> TableRegistry::findOrCreateTable(const EntityType type,
                                                            ComponentRegistry& componentRegistry) noexcept {
    bool isCreated = false;
    return this->findOrCreateTable(type, componentRegistry, isCreated);
}

std::pair<TableId, Table&> TableRegistry::findOrCreateTable(const EntityType type,
                                                            ComponentRegistry& componentRegistry,
                                                            bool& isCreated) noexcept {
    auto [tid, table] = this->tables_map.findOrCreate(type, componentRegistry, isCreated);

    if (isCreated) {
        for (const auto& hook : this->onTableCreated) {
            hook(table, tid);
        }
    }

    return {tid, table};
}

std::vector<Table>& TableRegistry::getTables() noexcept {
    return this->tables_map.tables;
}
