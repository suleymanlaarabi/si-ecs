#include <criterion/criterion.h>

#include "ecs/System.hpp"
#include "ecs/World.hpp"

namespace {
struct ShrinkPosition {
    int x = 0;
    int y = 0;
};

struct ShrinkVelocity {
    int dx = 0;
    int dy = 0;
};

struct ShrinkBoost {
    int value = 0;
};

struct ShrinkShield {
    int value = 0;
};

struct ShrinkTag {
    int value = 0;
};
}

Test(world_shrink, shrink_keeps_registered_system_queries_valid_after_removing_empty_swapped_table) {
    World world;
    world.registerComponent<ShrinkPosition>();
    world.registerComponent<ShrinkVelocity>();
    world.registerComponent<ShrinkBoost>();
    world.registerComponent<ShrinkShield>();

    int movingRuns = 0;
    int boostedRuns = 0;

    world.system(Phase::Update, each([&](ShrinkPosition& position, ShrinkVelocity& velocity) {
        movingRuns += 1;
        position.x += velocity.dx;
        position.y += velocity.dy;
    }));

    world.system(Phase::Update, each([&](ShrinkPosition& position, ShrinkVelocity& velocity, ShrinkBoost& boost) {
        boostedRuns += 1;
        position.x += boost.value;
        position.y += velocity.dy;
    }));

    const Entity base = world.createEntity();
    const Entity doomedBoost = world.createEntity();
    const Entity swappedShield = world.createEntity();
    const Entity positionOnly = world.createEntity();

    world.set(base, ShrinkPosition{.x = 1, .y = 2});
    world.set(base, ShrinkVelocity{.dx = 10, .dy = 20});

    world.set(doomedBoost, ShrinkPosition{.x = 3, .y = 4});
    world.set(doomedBoost, ShrinkVelocity{.dx = 30, .dy = 40});
    world.set(doomedBoost, ShrinkBoost{.value = 7});

    world.set(swappedShield, ShrinkPosition{.x = 5, .y = 6});
    world.set(swappedShield, ShrinkVelocity{.dx = 50, .dy = 60});
    world.set(swappedShield, ShrinkShield{.value = 9});

    world.set(positionOnly, ShrinkPosition{.x = 100, .y = 200});

    world.progress();

    cr_assert_eq(movingRuns, 3);
    cr_assert_eq(boostedRuns, 1);
    cr_assert_eq(world.get<ShrinkPosition>(base).x, 11);
    cr_assert_eq(world.get<ShrinkPosition>(base).y, 22);
    cr_assert_eq(world.get<ShrinkPosition>(doomedBoost).x, 40);
    cr_assert_eq(world.get<ShrinkPosition>(doomedBoost).y, 84);
    cr_assert_eq(world.get<ShrinkPosition>(swappedShield).x, 55);
    cr_assert_eq(world.get<ShrinkPosition>(swappedShield).y, 66);
    cr_assert_eq(world.get<ShrinkPosition>(positionOnly).x, 100);
    cr_assert_eq(world.get<ShrinkPosition>(positionOnly).y, 200);

    const size_t tablesBeforeDestroy = world.getTables().size();
    world.destroyEntity(doomedBoost);

    movingRuns = 0;
    boostedRuns = 0;

    world.shrink();

    cr_assert_lt(world.getTables().size(), tablesBeforeDestroy);

    world.progress();

    cr_assert_eq(movingRuns, 2);
    cr_assert_eq(boostedRuns, 0);
    cr_assert(world.isAlive(base));
    cr_assert(world.isAlive(swappedShield));
    cr_assert(world.has<ShrinkShield>(swappedShield));
    cr_assert_eq(world.get<ShrinkPosition>(base).x, 21);
    cr_assert_eq(world.get<ShrinkPosition>(base).y, 42);
    cr_assert_eq(world.get<ShrinkPosition>(swappedShield).x, 105);
    cr_assert_eq(world.get<ShrinkPosition>(swappedShield).y, 126);
    cr_assert_eq(world.get<ShrinkPosition>(positionOnly).x, 100);
    cr_assert_eq(world.get<ShrinkPosition>(positionOnly).y, 200);
}

Test(world_shrink, shrink_invalidates_cached_table_transition_edges_for_component_mutations) {
    World world;
    world.registerComponent<ShrinkPosition>();
    world.registerComponent<ShrinkVelocity>();
    world.registerComponent<ShrinkShield>();
    world.registerComponent<ShrinkBoost>();

    const Entity positionOnly = world.createEntity();
    const Entity cachedShield = world.createEntity();
    const Entity mixed = world.createEntity();
    const Entity tail = world.createEntity();

    world.set(positionOnly, ShrinkPosition{.x = 1});
    world.set(cachedShield, ShrinkPosition{.x = 2});
    world.set(mixed, ShrinkPosition{.x = 3});
    world.set(tail, ShrinkPosition{.x = 4});

    world.set(cachedShield, ShrinkShield{.value = 7});

    world.set(mixed, ShrinkVelocity{.dx = 5});
    world.set(mixed, ShrinkShield{.value = 8});
    world.remove<ShrinkVelocity>(mixed);
    world.set(mixed, ShrinkVelocity{.dx = 6});

    world.set(tail, ShrinkBoost{.value = 9});

    world.destroyEntity(cachedShield);
    world.shrink();

    world.set(positionOnly, ShrinkShield{.value = 10});
    cr_assert(world.has<ShrinkShield>(positionOnly));
    cr_assert_not(world.has<ShrinkBoost>(positionOnly));
    cr_assert_not(world.has<ShrinkVelocity>(positionOnly));

    world.remove<ShrinkVelocity>(mixed);
    cr_assert(world.has<ShrinkShield>(mixed));
    cr_assert_not(world.has<ShrinkBoost>(mixed));
    cr_assert_not(world.has<ShrinkVelocity>(mixed));
}

Test(world_shrink, shrink_keeps_component_migrations_and_future_matching_queries_working) {
    World world;
    world.registerComponent<ShrinkPosition>();
    world.registerComponent<ShrinkVelocity>();
    world.registerComponent<ShrinkBoost>();
    world.registerComponent<ShrinkShield>();
    world.registerComponent<ShrinkTag>();

    int boostedRuns = 0;

    world.system(Phase::Update, each([&](ShrinkPosition& position, ShrinkVelocity& velocity, ShrinkBoost& boost) {
        boostedRuns += 1;
        position.x += velocity.dx + boost.value;
    }));

    const Entity base = world.createEntity();
    const Entity doomedBoost = world.createEntity();
    const Entity swappedShield = world.createEntity();
    const Entity tagOnly = world.createEntity();

    world.set(base, ShrinkPosition{.x = 0});
    world.set(base, ShrinkVelocity{.dx = 1});

    world.set(doomedBoost, ShrinkPosition{.x = 10});
    world.set(doomedBoost, ShrinkVelocity{.dx = 2});
    world.set(doomedBoost, ShrinkBoost{.value = 3});

    world.set(swappedShield, ShrinkPosition{.x = 20});
    world.set(swappedShield, ShrinkVelocity{.dx = 4});
    world.set(swappedShield, ShrinkShield{.value = 5});

    world.set(tagOnly, ShrinkTag{.value = 99});

    world.progress();

    cr_assert_eq(boostedRuns, 1);
    cr_assert_eq(world.get<ShrinkPosition>(doomedBoost).x, 15);

    const size_t tablesBeforeShrink = world.getTables().size();
    world.destroyEntity(doomedBoost);
    world.shrink();

    cr_assert_lt(world.getTables().size(), tablesBeforeShrink);
    cr_assert(world.isAlive(base));
    cr_assert(world.isAlive(swappedShield));
    cr_assert(world.has<ShrinkShield>(swappedShield));

    boostedRuns = 0;
    world.set(swappedShield, ShrinkBoost{.value = 6});

    cr_assert(world.has<ShrinkBoost>(swappedShield));
    cr_assert(world.has<ShrinkShield>(swappedShield));

    world.progress();

    cr_assert_eq(boostedRuns, 1);
    cr_assert_eq(world.get<ShrinkPosition>(swappedShield).x, 30);
    cr_assert_eq(world.get<ShrinkTag>(tagOnly).value, 99);
    cr_assert_eq(world.get<ShrinkPosition>(base).x, 0);
}
