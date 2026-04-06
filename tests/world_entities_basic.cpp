#include <criterion/criterion.h>

#include "ecs/core/base/EcsType.hpp"
#include "ecs/World.hpp"

namespace {
    struct Position {
        int x;
    };

    uint64_t componentTableSize(World& world, const ComponentId cid) {
        for (Table& table : world.getTables()) {
            if (table.hasComponent(cid)) {
                return table.size();
            }
        }

        return 0;
    }
}

Test(world, create_destroy_and_liveness_follow_entity_generation) {
    World world;

    const Entity first = world.createEntity();
    const Entity second = world.createEntity();

    cr_assert(first.index != 0);
    cr_assert(second.index != 0);
    cr_assert_neq(first.index, second.index);
    cr_assert(world.isAlive(first));
    cr_assert(world.isAlive(second));

    world.destroyEntity(first);

    cr_assert_not(world.isAlive(first));
    cr_assert(world.isAlive(second));

    const Entity recycled = world.createEntity();

    cr_assert_eq(recycled.index, first.index);
    cr_assert_neq(recycled.generation, first.generation);
    cr_assert_not(world.isAlive(first));
    cr_assert(world.isAlive(recycled));
}

Test(world, destroy_entity_removes_component_row_and_keeps_other_entities_valid) {
    World world;
    world.registerComponent<Position>();

    const ComponentId positionId = ComponentRegistry::id<Position>();
    const Entity first = world.createEntity();
    const Entity second = world.createEntity();

    world.add<Position>(first);
    world.add<Position>(second);
    world.get<Position>(first).x = 10;
    world.get<Position>(second).x = 20;

    cr_assert_eq(componentTableSize(world, positionId), 2);

    world.destroyEntity(first);

    cr_assert_not(world.isAlive(first));
    cr_assert_eq(componentTableSize(world, positionId), 1);
    cr_assert_eq(world.get<Position>(second).x, 20);

    const Entity recycled = world.createEntity();
    world.add<Position>(recycled);
    world.get<Position>(recycled).x = 30;

    cr_assert_eq(componentTableSize(world, positionId), 2);
    cr_assert_eq(world.get<Position>(second).x, 20);
    cr_assert_eq(world.get<Position>(recycled).x, 30);
}
