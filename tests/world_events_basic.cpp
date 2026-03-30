#include <criterion/criterion.h>

#include "ecs/World.hpp"

struct CollisionStart {
    int impact = 0;
};

namespace {
    int g_collision_calls = 0;
    int g_last_impact = 0;

    void onCollisionStart(World&, const CollisionStart& event) {
        ++g_collision_calls;
        g_last_impact = event.impact;
    }
}

Test(world, listen_emit_and_unlisten_event_callbacks) {
    g_collision_calls = 0;
    g_last_impact = 0;

    World world;

    const Entity entity = world.createEntity();
    cr_assert(world.isAlive(entity));

    const EventId id = world.listen<CollisionStart>(onCollisionStart);

    world.emit(CollisionStart{.impact = 12});

    cr_assert_eq(g_collision_calls, 1);
    cr_assert_eq(g_last_impact, 12);

    world.unlisten<CollisionStart>(id);
    world.emit(CollisionStart{.impact = 99});

    cr_assert_eq(g_collision_calls, 1);
    cr_assert_eq(g_last_impact, 12);
}
