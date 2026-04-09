#include <criterion/criterion.h>

#include "ecs/core/entity/EntityManager.hpp"

struct Position {
    int x;
    int y;
};

struct Velocity {
    int dx;
    int dy;
};

Test(entity_manager, add_get_remove_component_keeps_existing_data) {
    EntityManager manager;
    manager.registerComponent<Position>();
    manager.registerComponent<Velocity>();

    const ComponentId positionId = ComponentRegistry::id<Position>();
    const ComponentId velocityId = ComponentRegistry::id<Velocity>();

    const Entity entity = manager.createEntity();

    manager.addComponent(entity, positionId);
    auto* position = static_cast<Position*>(manager.getComponent(entity, positionId));

    cr_assert_not_null(position);
    position->x = 10;
    position->y = 20;

    manager.addComponent(entity, velocityId);

    position = static_cast<Position*>(manager.getComponent(entity, positionId));
    auto* velocity = static_cast<Velocity*>(manager.getComponent(entity, velocityId));

    cr_assert_not_null(position);
    cr_assert_not_null(velocity);
    cr_assert(position->x == 10);
    cr_assert(position->y == 20);

    velocity->dx = 3;
    velocity->dy = 4;

    manager.removeComponent(entity, positionId);

    velocity = static_cast<Velocity*>(manager.getComponent(entity, velocityId));
    cr_assert_not_null(velocity);
    cr_assert(velocity->dx == 3);
    cr_assert(velocity->dy == 4);
}
