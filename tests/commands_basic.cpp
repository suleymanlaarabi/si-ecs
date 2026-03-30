#include <criterion/criterion.h>

#include "ecs/Commands.hpp"
#include "ecs/System.hpp"
#include "ecs/World.hpp"

namespace {
    struct Position {
        int x = 0;
        int y = 0;
    };

    struct Velocity {
        int x = 0;
        int y = 0;
    };

    struct Health {
        int value = 0;
    };
}

Test(commands, commands_param_is_resolved_and_deferred_add_is_visible_in_later_phase) {
    World world;
    world.registerComponent<Position>();
    world.registerComponent<Health>();

    const Entity entity = world.createEntity();
    world.set(entity, Position{.x = 1});

    int updateRuns = 0;
    int postUpdateRuns = 0;

    world.system(Phase::Update, each([&updateRuns](Entity current, Position&, Commands& commands) {
        updateRuns += 1;
        commands.add<Health>(current);
    }));

    world.system(Phase::PostUpdate, each([&postUpdateRuns](Health&) {
        postUpdateRuns += 1;
    }));

    world.progress();

    cr_assert_eq(updateRuns, 1);
    cr_assert_eq(postUpdateRuns, 1);
    cr_assert(world.has<Health>(entity));
}

Test(commands, deferred_remove_is_applied_once_and_cleared_after_flush) {
    World world;
    world.registerComponent<Position>();

    const Entity entity = world.createEntity();
    world.set(entity, Position{.x = 10});

    int updateRuns = 0;

    world.system(Phase::Update, each([&updateRuns](Entity current, Position&, Commands& commands) {
        updateRuns += 1;
        commands.remove<Position>(current);
    }));

    world.progress();

    cr_assert_eq(updateRuns, 1);
    cr_assert_not(world.has<Position>(entity));

    world.progress();

    cr_assert_eq(updateRuns, 1);
    cr_assert_not(world.has<Position>(entity));
}

Test(commands, deferred_set_copies_value_before_flush) {
    World world;
    world.registerComponent<Position>();
    world.registerComponent<Health>();

    const Entity entity = world.createEntity();
    world.set(entity, Position{.x = 0});

    world.system(Phase::Update, each([](Entity current, Position&, Commands& commands) {
        Health health{.value = 42};
        commands.set<Health>(current, health);
        health.value = 7;
    }));

    world.progress();

    cr_assert(world.has<Health>(entity));
    cr_assert_eq(world.get<Health>(entity).value, 42);
}
