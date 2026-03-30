#include <criterion/criterion.h>

#include "ecs/EntityManager.hpp"

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

struct Acceleration {
    int x;
    int y;
};

struct BatchHealth {
    int value;
};

struct BatchNeedsHealth : Required<BatchHealth> {};

Test(entity_manager, add_batch_components_migrates_once_and_keeps_existing_data) {
    EntityManager manager;
    manager.registerComponent<Position>();
    manager.registerComponent<Velocity>();
    manager.registerComponent<Acceleration>();

    const ComponentId positionId = ComponentRegistry::id<Position>();
    const ComponentId velocityId = ComponentRegistry::id<Velocity>();
    const ComponentId accelerationId = ComponentRegistry::id<Acceleration>();

    const Entity entity = manager.createEntity();

    manager.addComponent(entity, positionId);
    auto* position = static_cast<Position*>(manager.getComponent(entity, positionId));
    position->x = 7;
    position->y = 9;

    const ComponentId batch[] = {accelerationId, velocityId};
    manager.addBatchComponents(entity, batch, 2);

    position = static_cast<Position*>(manager.getComponent(entity, positionId));
    auto* velocity = static_cast<Velocity*>(manager.getComponent(entity, velocityId));
    auto* acceleration = static_cast<Acceleration*>(manager.getComponent(entity, accelerationId));

    cr_assert_not_null(position);
    cr_assert_not_null(velocity);
    cr_assert_not_null(acceleration);
    cr_assert_eq(position->x, 7);
    cr_assert_eq(position->y, 9);
}

Test(entity_manager, add_batch_components_adds_required_components_and_preserves_existing_values) {
    EntityManager manager;
    manager.registerComponent<Position>();
    manager.registerComponent<BatchHealth>();
    manager.registerComponent<BatchNeedsHealth>();

    const ComponentId positionId = ComponentRegistry::id<Position>();
    const ComponentId needsHealthId = ComponentRegistry::id<BatchNeedsHealth>();

    const Entity entity = manager.createEntity();

    manager.addComponent(entity, positionId);
    auto* position = static_cast<Position*>(manager.getComponent(entity, positionId));
    position->x = 21;
    position->y = 34;

    manager.addBatchComponents(entity, &needsHealthId, 1);

    position = static_cast<Position*>(manager.getComponent(entity, positionId));
    auto* health = static_cast<BatchHealth*>(manager.getComponent(entity, ComponentRegistry::id<BatchHealth>()));

    cr_assert_not_null(position);
    cr_assert_not_null(health);
    cr_assert_eq(position->x, 21);
    cr_assert_eq(position->y, 34);
    cr_assert(manager.hasComponent(entity, ComponentRegistry::id<BatchHealth>()));
    cr_assert(manager.hasComponent(entity, needsHealthId));
}
