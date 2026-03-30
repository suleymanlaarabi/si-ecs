#include <criterion/criterion.h>

#include "ecs/World.hpp"

struct Health {
    int value;
};

Test(world, add_has_get_remove_component_through_world_api) {
    World world;
    world.registerComponent<Health>();

    const Entity entity = world.createEntity();

    cr_assert_not(world.has<Health>(entity));

    world.add<Health>(entity);

    cr_assert(world.has<Health>(entity));

    Health& health = world.get<Health>(entity);
    health.value = 42;

    cr_assert_eq(world.get<Health>(entity).value, 42);

    world.remove<Health>(entity);

    cr_assert_not(world.has<Health>(entity));
}
