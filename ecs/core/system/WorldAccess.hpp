#pragma once

#include <utility>
#include <vector>

#include "EcsType.hpp"

class Table;
class World;

std::vector<Table*>& worldTables(World& world);
std::pair<Table&, EntityRow> worldGetTable(World& world, Entity entity);
