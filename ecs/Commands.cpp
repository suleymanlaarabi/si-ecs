#include "Commands.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "World.hpp"


Commands::Commands(World& world) : world(world) {}

void Commands::add(const Entity entity, const ComponentId cid) {
    this->adds.push_back({entity, cid});
}

void Commands::remove(const Entity entity, const ComponentId cid) {
    this->removes.push_back({entity, cid});
}

void Commands::set(const Entity entity, const ComponentId cid, const void* value, const size_t size) {
    void* allocated = this->buffer.allocate(size);
    memcpy(allocated, value, size);
    this->sets.push_back({entity, cid, allocated});
}

void Commands::flush() {
    for (const auto& [entity, cid] : this->adds) {
        world.addComponent(entity, cid);
    }
    for (const auto& [entity, cid] : this->removes) {
        world.removeComponent(entity, cid);
    }
    for (const auto& [entity, cid, value] : this->sets) {
        world.setComponent(entity, cid, value);
    }
    for (const Entity entity : this->kills) {
        world.destroyEntity(entity);
    }

    this->adds.size = 0;
    this->removes.size = 0;
    this->sets.size = 0;
    this->kills.size = 0;
    this->buffer.release();
}

Entity Commands::spawn() const {
    return world.createEntity();
}

Commands& Commands::fromWorldTable(World& world, const Table&) {
    return world.getSingleton<Commands>();
}

void Commands::kill(const Entity entity) {
    this->kills.push_back(entity);
}
