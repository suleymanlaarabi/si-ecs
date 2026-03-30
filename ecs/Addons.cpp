#include "Addons.hpp"
#include "World.hpp"

DeltaTime& DeltaTime::fromWorldTable(World& world, const Table& table) {
    return world.getSingleton<DeltaTime>();
}
