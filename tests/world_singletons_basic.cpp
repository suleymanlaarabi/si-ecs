#include <criterion/criterion.h>

#include "ecs/World.hpp"

struct GameConfig {
    int maxPlayers = 4;
};

Test(world, singleton_is_absent_before_initialization_and_present_after_default_init) {
    World world;

    cr_assert_not(world.hasSingleton<GameConfig>());

    world.initSingleton<GameConfig>();

    cr_assert(world.hasSingleton<GameConfig>());

    GameConfig& config = world.getSingleton<GameConfig>();
    cr_assert_eq(config.maxPlayers, 4);

    config.maxPlayers = 8;
    cr_assert_eq(world.getSingleton<GameConfig>().maxPlayers, 8);
}

Test(world, singleton_can_be_initialized_from_external_pointer) {
    World world;
    GameConfig config{.maxPlayers = 16};

    world.initSingleton<GameConfig>(&config);

    cr_assert(world.hasSingleton<GameConfig>());
    cr_assert_eq(&world.getSingleton<GameConfig>(), &config);
    cr_assert_eq(world.getSingleton<GameConfig>().maxPlayers, 16);

    world.getSingleton<GameConfig>().maxPlayers = 32;
    cr_assert_eq(config.maxPlayers, 32);
}
