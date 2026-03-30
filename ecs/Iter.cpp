#include "Iter.hpp"

#include "QueryTables.hpp"
#include "Table.hpp"
#include "World.hpp"

RowView::RowView(World& world, const Table& table, const EntityRow row) : world(&world), table(&table), row(row) {}

Entity RowView::entity() const {
    return this->table->getEntities()[this->row];
}

bool RowView::has(const ComponentId cid) const {
    return this->table->hasComponent(cid);
}

void* RowView::get(const ComponentId cid) const {
    return this->table->getComponent(this->row, cid);
}

RuntimeQuery::RuntimeQuery(World& world, Query query) : world(&world), desc(std::move(query)) {}

RuntimeQuery& RuntimeQuery::with(const ComponentId cid) {
    this->desc.with(cid);
    return *this;
}

RuntimeQuery& RuntimeQuery::without(const ComponentId cid) {
    this->desc.without(cid);
    return *this;
}

void RuntimeQuery::eachRow(const std::function<void(RowView)>& func) const {
    this->world->eachMatchingTable(this->desc, [&](const TableId, Table& table) {
        for (EntityRow row = 0; row < table.size(); ++row) {
            func(RowView(*this->world, table, row));
        }
    });
}
