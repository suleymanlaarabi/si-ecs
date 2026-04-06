#include <criterion/criterion.h>

#include "ecs/Commands.hpp"
#include <vector>

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

    struct Mass {
        int value = 0;
    };

    struct Health {
        int value = 0;
    };

    struct Sleeping {};

    struct Bonus {};

    struct DeltaTime {
        int value = 1;

        static const DeltaTime &fromWorldTable(World &world, const Table &) {
            return world.getSingleton<DeltaTime>();
        }
    };

    struct RowEntity {
        Entity value;

        static RowEntity fromWorldRow(World &, const Table &table, const EntityRow row) {
            return RowEntity{.value = table.getEntities()[row]};
        }
    };

    template<auto func>
    SystemDesc describeSystem(const Query &query) {
        return {
            .query = query,
            .callback = +[](const EcsVec<TableId>& tables, World& world) {
                func(tables, world);
            }
        };
    }

    Test(system, each_updates_only_entities_matching_all_required_components) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.registerComponent<Mass>();

        const Entity moving = world.createEntity();
        const Entity stationary = world.createEntity();
        const Entity unrelated = world.createEntity();

        world.set(moving, Position{.x = 1, .y = 2});
        world.set(moving, Velocity{.x = 3, .y = 4});

        world.set(stationary, Position{.x = 10, .y = 20});
        world.set(unrelated, Mass{.value = 5});

        world.system(Phase::Update, each<[](Position &pos, Velocity &vel) {
            pos.x += vel.x;
            pos.y += vel.y;
        }>());

        world.progress();

        cr_assert_eq(world.get<Position>(moving).x, 4);
        cr_assert_eq(world.get<Position>(moving).y, 6);
        cr_assert_eq(world.get<Position>(stationary).x, 10);
        cr_assert_eq(world.get<Position>(stationary).y, 20);
        cr_assert_eq(world.get<Mass>(unrelated).value, 5);
    }

    Test(system, each_visits_all_matching_entities_across_multiple_tables) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.registerComponent<Bonus>();

        const Entity base = world.createEntity();
        const Entity bonus = world.createEntity();
        const Entity secondBonus = world.createEntity();
        const Entity positionOnly = world.createEntity();

        world.set(base, Position{.x = 0, .y = 0});
        world.set(base, Velocity{.x = 1, .y = 2});

        world.set(bonus, Position{.x = 10, .y = 20});
        world.set(bonus, Velocity{.x = 3, .y = 4});
        world.add<Bonus>(bonus);

        world.set(secondBonus, Position{.x = -5, .y = -7});
        world.set(secondBonus, Velocity{.x = 6, .y = 8});
        world.add<Bonus>(secondBonus);

        world.set(positionOnly, Position{.x = 99, .y = 100});

        world.system(Phase::Update, each<[](Position &pos, Velocity &vel) {
            pos.x += vel.x;
            pos.y += vel.y;
        }>());

        world.progress();

        cr_assert_eq(world.get<Position>(base).x, 1);
        cr_assert_eq(world.get<Position>(base).y, 2);
        cr_assert_eq(world.get<Position>(bonus).x, 13);
        cr_assert_eq(world.get<Position>(bonus).y, 24);
        cr_assert_eq(world.get<Position>(secondBonus).x, 1);
        cr_assert_eq(world.get<Position>(secondBonus).y, 1);
        cr_assert_eq(world.get<Position>(positionOnly).x, 99);
        cr_assert_eq(world.get<Position>(positionOnly).y, 100);
    }

    Test(system, registered_query_picks_up_matching_tables_created_after_system_registration) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();

        static int runs = 0;
        world.system(Phase::Update, each<[](Position &pos, Velocity &vel) {
            runs += 1;
            pos.x += vel.x;
        }>());

        const Entity first = world.createEntity();
        const Entity second = world.createEntity();

        world.set(first, Position{.x = 5});
        world.set(first, Velocity{.x = 2});
        world.set(second, Position{.x = 8});
        world.set(second, Velocity{.x = -3});

        world.progress();

        cr_assert_eq(runs, 2);
        cr_assert_eq(world.get<Position>(first).x, 7);
        cr_assert_eq(world.get<Position>(second).x, 5);
    }

    Test(system, each_supports_without_filters_across_multiple_tables) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.registerComponent<Sleeping>();
        world.registerComponent<Bonus>();

        const Entity awake = world.createEntity();
        const Entity awakeBonus = world.createEntity();
        const Entity sleeping = world.createEntity();
        const Entity sleepingBonus = world.createEntity();

        world.set(awake, Position{.x = 1});
        world.set(awake, Velocity{.x = 10});

        world.set(awakeBonus, Position{.x = 2});
        world.set(awakeBonus, Velocity{.x = 20});
        world.add<Bonus>(awakeBonus);

        world.set(sleeping, Position{.x = 3});
        world.set(sleeping, Velocity{.x = 30});
        world.add<Sleeping>(sleeping);

        world.set(sleepingBonus, Position{.x = 4});
        world.set(sleepingBonus, Velocity{.x = 40});
        world.add<Bonus>(sleepingBonus);
        world.add<Sleeping>(sleepingBonus);

        static int matchedEntities = 0;

        world.system(Phase::Update, each<[](Position &pos, Velocity &vel) {
            matchedEntities += 1;
            pos.x += vel.x;
        }, Without<Sleeping> >());

        world.progress();

        cr_assert_eq(matchedEntities, 2);
        cr_assert_eq(world.get<Position>(awake).x, 11);
        cr_assert_eq(world.get<Position>(awakeBonus).x, 22);
        cr_assert_eq(world.get<Position>(sleeping).x, 3);
        cr_assert_eq(world.get<Position>(sleepingBonus).x, 4);
    }

    Test(system, systems_in_same_phase_run_in_registration_order) {
        World world;
        static std::vector<int> calls;

        world.system(Phase::Update, describeSystem<[](const EcsVec<TableId> &, World &) {
            calls.push_back(1);
        }>(query<>()));
        world.system(Phase::Update, describeSystem<[](const EcsVec<TableId> &, World &) {
            calls.push_back(2);
        }>(query<>()));
        world.system(Phase::Update, describeSystem<[](const EcsVec<TableId> &, World &) {
            calls.push_back(3);
        }>(query<>()));

        world.progress();

        cr_assert_eq(calls.size(), 3);
        cr_assert_eq(calls[0], 1);
        cr_assert_eq(calls[1], 2);
        cr_assert_eq(calls[2], 3);
    }

    Test(system, each_handles_large_numbers_of_entities) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.registerComponent<Bonus>();

        int expectedSum = 0;
        int unmatchedSum = 0;

        for (int i = 0; i < 2000; ++i) {
            const Entity entity = world.createEntity();
            world.set(entity, Position{.x = i, .y = i * 2});

            if ((i % 2) == 0) {
                world.set(entity, Velocity{.x = 1, .y = -1});
                expectedSum += i + 1;
                if ((i % 4) == 0) {
                    world.add<Bonus>(entity);
                }
            } else {
                unmatchedSum += i;
                if ((i % 3) == 0) {
                    world.add<Bonus>(entity);
                }
            }
        }

        static int updatedCount = 0;
        static int updatedSum = 0;

        world.system(Phase::Update, each<[](Position &pos, Velocity &vel) {
            pos.x += vel.x;
            pos.y += vel.y;
            updatedCount += 1;
            updatedSum += pos.x;
        }>());

        world.progress();

        int unmatchedCount = 0;
        int observedUnmatchedSum = 0;
        for (int i = 0; i < 2000; ++i) {
            if ((i % 2) != 0) {
                observedUnmatchedSum += i;
                unmatchedCount += 1;
            }
        }

        cr_assert_eq(updatedCount, 1000);
        cr_assert_eq(updatedSum, expectedSum);
        cr_assert_eq(unmatchedCount, 1000);
        cr_assert_eq(observedUnmatchedSum, unmatchedSum);
    }

    Test(system, each_entity_exposes_the_matching_entity_to_the_callback) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();

        const Entity first = world.createEntity();
        const Entity second = world.createEntity();
        const Entity positionOnly = world.createEntity();

        world.set(first, Position{.x = 1, .y = 2});
        world.set(first, Velocity{.x = 10, .y = 20});

        world.set(second, Position{.x = 3, .y = 4});
        world.set(second, Velocity{.x = 30, .y = 40});

        world.set(positionOnly, Position{.x = 9, .y = 9});

        static std::vector<Entity> visited;

        world.system(Phase::Update, each<[](Entity entity, Position &pos, Velocity &vel) {
            visited.push_back(entity);
            pos.x += vel.x;
        }>());

        world.progress();

        cr_assert_eq(visited.size(), 2);
        cr_assert((visited[0] == first && visited[1] == second) || (visited[0] == second && visited[1] == first));
        cr_assert_eq(world.get<Position>(first).x, 11);
        cr_assert_eq(world.get<Position>(second).x, 33);
        cr_assert_eq(world.get<Position>(positionOnly).x, 9);
    }

    Test(system, each_entity_visits_entities_across_multiple_matching_tables) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.registerComponent<Bonus>();

        const Entity base = world.createEntity();
        const Entity bonus = world.createEntity();
        const Entity anotherBonus = world.createEntity();

        world.set(base, Position{.x = 0});
        world.set(base, Velocity{.x = 1});

        world.set(bonus, Position{.x = 10});
        world.set(bonus, Velocity{.x = 2});
        world.add<Bonus>(bonus);

        world.set(anotherBonus, Position{.x = 20});
        world.set(anotherBonus, Velocity{.x = 3});
        world.add<Bonus>(anotherBonus);

        static std::vector<Entity> visited;

        world.system(Phase::Update, each<[](Entity entity, Position &pos, Velocity &vel) {
            visited.push_back(entity);
            pos.x += vel.x;
        }>());

        world.progress();

        cr_assert_eq(visited.size(), 3);
        cr_assert_eq(world.get<Position>(base).x, 1);
        cr_assert_eq(world.get<Position>(bonus).x, 12);
        cr_assert_eq(world.get<Position>(anotherBonus).x, 23);
    }

    Test(system, each_entity_entity_argument_matches_the_world_storage_after_row_swaps) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();

        const Entity first = world.createEntity();
        const Entity second = world.createEntity();
        const Entity third = world.createEntity();

        world.set(first, Position{.x = 1});
        world.set(first, Velocity{.x = 10});
        world.set(second, Position{.x = 2});
        world.set(second, Velocity{.x = 20});
        world.set(third, Position{.x = 3});
        world.set(third, Velocity{.x = 30});

        world.destroyEntity(second);

        world.system(Phase::Update, each<[](const Entity entity, Position &pos, Velocity &vel) {
            pos.y = static_cast<int>(entity.index);
            pos.x += vel.x;
        }>());

        world.progress();

        cr_assert_eq(world.get<Position>(first).x, 11);
        cr_assert_eq(world.get<Position>(third).x, 33);
        cr_assert_eq(world.get<Position>(first).y, static_cast<int>(first.index));
        cr_assert_eq(world.get<Position>(third).y, static_cast<int>(third.index));
    }

    Test(system, each_entity_handles_large_numbers_of_entities) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.registerComponent<Bonus>();

        static int visited = 0;
        static uint64_t indexSum = 0;
        static int expectedUpdated = 0;

        for (int i = 0; i < 3000; ++i) {
            const Entity entity = world.createEntity();
            world.set(entity, Position{.x = i});

            if ((i % 3) != 0) {
                world.set(entity, Velocity{.x = 2});
                expectedUpdated += i + 2;
                if ((i % 5) == 0) {
                    world.add<Bonus>(entity);
                }
            } else {
                if ((i % 7) == 0) {
                    world.add<Bonus>(entity);
                }
            }
        }

        static int observedUpdated = 0;

        world.system(Phase::Update, each<[](Entity entity, Position &pos, Velocity &vel) {
            visited += 1;
            indexSum += entity.index;
            pos.x += vel.x;
            observedUpdated += pos.x;
        }>());

        world.progress();

        cr_assert_eq(visited, 2000);
        cr_assert_gt(indexSum, 0);
        cr_assert_eq(observedUpdated, expectedUpdated);
    }

    Test(system, each_supports_entity_components_and_without_combination) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.registerComponent<Sleeping>();

        const Entity awake = world.createEntity();
        const Entity sleeping = world.createEntity();

        world.set(awake, Position{.x = 1});
        world.set(awake, Velocity{.x = 2});

        world.set(sleeping, Position{.x = 10});
        world.set(sleeping, Velocity{.x = 20});
        world.add<Sleeping>(sleeping);

        static std::vector<Entity> visited;

        world.system(Phase::Update, each<[](Entity entity, Position &pos, Velocity &vel) {
            visited.push_back(entity);
            pos.x += vel.x;
        }, Without<Sleeping> >());

        world.progress();

        cr_assert_eq(visited.size(), 1);
        cr_assert_eq(visited[0], awake);
        cr_assert_eq(world.get<Position>(awake).x, 3);
        cr_assert_eq(world.get<Position>(sleeping).x, 10);
    }

    Test(system, each_supports_components_with_delta_time_param) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.initSingleton<DeltaTime>();
        world.getSingleton<DeltaTime>().value = 3;

        const Entity entity = world.createEntity();
        world.set(entity, Position{.x = 1, .y = 2});
        world.set(entity, Velocity{.x = 4, .y = 5});

        world.system(Phase::Update, each<[](Position &pos, Velocity &vel, const DeltaTime &dt) {
            pos.x += vel.x * dt.value;
            pos.y += vel.y * dt.value;
        }>());

        world.progress();

        cr_assert_eq(world.get<Position>(entity).x, 13);
        cr_assert_eq(world.get<Position>(entity).y, 17);
    }

    Test(system, each_supports_entity_commands_and_without_combination) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Health>();
        world.registerComponent<Sleeping>();

        const Entity awake = world.createEntity();
        const Entity sleeping = world.createEntity();

        world.set(awake, Position{.x = 1});
        world.set(sleeping, Position{.x = 2});
        world.add<Sleeping>(sleeping);

        world.system(Phase::Update, each<[](Entity entity, Position &, Commands &commands) {
            commands.add<Health>(entity);
        }, Without<Sleeping> >());

        world.progress();

        cr_assert(world.has<Health>(awake));
        cr_assert_not(world.has<Health>(sleeping));
    }

    Test(system, each_supports_const_component_params) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();

        const Entity entity = world.createEntity();
        world.set(entity, Position{.x = 2, .y = 3});
        world.set(entity, Velocity{.x = 5, .y = 7});

        static int sum = 0;

        world.system(Phase::Update, each<[](const Position &pos, const Velocity &vel) {
            sum += pos.x + pos.y + vel.x + vel.y;
        }>());

        world.progress();

        cr_assert_eq(sum, 17);
    }

    Test(system, each_supports_from_world_row_params) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();

        const Entity first = world.createEntity();
        const Entity second = world.createEntity();

        world.set(first, Position{.x = 1});
        world.set(first, Velocity{.x = 2});
        world.set(second, Position{.x = 10});
        world.set(second, Velocity{.x = 20});

        static std::vector<Entity> visited;

        world.system(Phase::Update, each<[](RowEntity rowEntity, Position &pos, Velocity &vel) {
            visited.push_back(rowEntity.value);
            pos.x += vel.x;
        }>());

        world.progress();

        cr_assert_eq(visited.size(), 2);
        cr_assert((visited[0] == first && visited[1] == second) || (visited[0] == second && visited[1] == first));
        cr_assert_eq(world.get<Position>(first).x, 3);
        cr_assert_eq(world.get<Position>(second).x, 30);
    }

    Test(system, each_supports_from_world_row_with_without_filters) {
        World world;
        world.registerComponent<Position>();
        world.registerComponent<Velocity>();
        world.registerComponent<Sleeping>();

        const Entity awake = world.createEntity();
        const Entity sleeping = world.createEntity();

        world.set(awake, Position{.x = 1});
        world.set(awake, Velocity{.x = 2});

        world.set(sleeping, Position{.x = 10});
        world.set(sleeping, Velocity{.x = 20});
        world.add<Sleeping>(sleeping);

        static std::vector<Entity> visited;

        world.system(Phase::Update, each<[](RowEntity rowEntity, Position &pos) {
            visited.push_back(rowEntity.value);
            pos.y = static_cast<int>(rowEntity.value.index);
        }, Without<Sleeping> >());

        world.progress();

        cr_assert_eq(visited.size(), 1);
        cr_assert_eq(visited[0], awake);
        cr_assert_eq(world.get<Position>(awake).y, static_cast<int>(awake.index));
        cr_assert_eq(world.get<Position>(sleeping).y, 0);
    }
}
