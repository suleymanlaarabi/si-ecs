#include "EntityRef.hpp"

#include "World.hpp"

EntityRef::EntityRef(World& world, const Entity entity) : world(world), entity(entity) {}


void EntityRef::add_id(const ComponentId cid) const {
    this->world.addComponent(this->entity, cid);
}


void EntityRef::set_id(const ComponentId cid, const void* value) const {
    this->world.setComponent(this->entity, cid, value);
}
