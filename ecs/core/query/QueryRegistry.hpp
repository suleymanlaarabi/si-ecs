#pragma once
#include <vector>
#include "EcsType.hpp"
#include "Query.hpp"

class ComponentRegistry;
class TableRegistry;

class QueryRegistry {
    std::vector<QueryCache> queries;

public:
    QueryId registerQuery(const Query& query, ComponentRegistry& componentRegistry, TableRegistry& tableRegistry);
    std::vector<QueryCache>& getQueries();
};
