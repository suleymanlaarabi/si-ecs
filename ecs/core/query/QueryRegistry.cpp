#include "QueryRegistry.hpp"
#include "Query.hpp"
#include "QueryTables.hpp"

QueryId QueryRegistry::registerQuery(const Query& query, ComponentRegistry& componentRegistry,
                                     TableRegistry& tableRegistry) {
    this->queries.emplace_back(QueryCache{query});
    QueryCache& cached = this->queries.back();
    forEachMatchingTable(cached.query, componentRegistry, tableRegistry, [&cached](const TableId tid, const Table&) {
        cached.tables.push_back(tid);
    });

    QueryId qid = this->queries.size() - 1;

    tableRegistry.onTableCreated.emplace_back([this, qid](Table& table, TableId tableId) {
        auto& cached = this->queries[qid];
        if (cached.query.matchTable(table)) {
            cached.tables.push_back(tableId);
        }
    });

    return qid;
}

std::vector<QueryCache>& QueryRegistry::getQueries() {
    return this->queries;
}
