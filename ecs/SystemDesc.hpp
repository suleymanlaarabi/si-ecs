#pragma once

#include <functional>

#include "EcsVec.hpp"
#include "Query.hpp"

class World;

struct SystemDesc {
    Query query;
    std::function<void(const EcsVec<TableId>& tables, World&)> callback;
};
