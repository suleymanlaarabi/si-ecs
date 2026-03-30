#include <criterion/criterion.h>

#include "ecs/Query.hpp"
#include "ecs/World.hpp"

struct Position {};

struct Velocity {};

struct NoIntegrate;
struct Sleeping {};

namespace {
struct RuntimePosition {
    int x = 0;
};

struct RuntimeVelocity {
    int x = 0;
};

struct RuntimeSleeping {};
}

Test(query, runtime_builder_supports_typed_and_untyped_terms_without_duplicates) {
    Query result;

    result.with<Position>()
        .with(ComponentRegistry::id<Velocity>())
        .with<Position>()
        .without<Sleeping>()
        .without(ComponentRegistry::id<Sleeping>());

    cr_assert_eq(result.required_count, 2);
    cr_assert_eq(result.required[0], ComponentRegistry::id<Position>());
    cr_assert_eq(result.required[1], ComponentRegistry::id<Velocity>());
    cr_assert_eq(result.excluded_count, 1);
    cr_assert_eq(result.excluded[0], ComponentRegistry::id<Sleeping>());
}

Test(query, world_runtime_query_iterates_matching_entities_without_cache_registration) {
    World world;
    world.registerComponent<Position>();
    world.registerComponent<Velocity>();
    world.registerComponent<Sleeping>();

    const Entity moving = world.createEntity();
    const Entity sleeping = world.createEntity();
    const Entity velocityOnly = world.createEntity();

    world.set(moving, Position{});
    world.set(moving, Velocity{});

    world.set(sleeping, Position{});
    world.set(sleeping, Velocity{});
    world.add<Sleeping>(sleeping);

    world.set(velocityOnly, Velocity{});

    uint32_t matched = 0;
    world.query()
        .with<Position>()
        .with<Velocity>()
        .without<Sleeping>()
        .each([&matched](const RowView row) {
            cr_assert(row.has<Position>());
            cr_assert(row.has<Velocity>());
            cr_assert_not(row.has<Sleeping>());
            row.get<Position>() = Position{};
            matched += 1;
        });

    cr_assert_eq(matched, 1);
    cr_assert_eq(world.getQueries().size(), 0);
}

Test(query, runtime_query_row_view_exposes_entity_get_and_try_get) {
    World world;
    world.registerComponent<RuntimePosition>();
    world.registerComponent<RuntimeVelocity>();
    world.registerComponent<RuntimeSleeping>();

    const Entity moving = world.createEntity();
    const Entity sleeping = world.createEntity();

    world.set(moving, RuntimePosition{.x = 10});
    world.set(moving, RuntimeVelocity{.x = 4});

    world.set(sleeping, RuntimePosition{.x = 50});
    world.set(sleeping, RuntimeVelocity{.x = 9});
    world.add<RuntimeSleeping>(sleeping);

    Entity visited = {0, 0};
    int positionSum = 0;

    world.query()
        .with<RuntimePosition>()
        .with<RuntimeVelocity>()
        .without<RuntimeSleeping>()
        .each([&](const RowView row) {
            visited = row.entity();
            cr_assert_eq(row.entity(), moving);
            cr_assert_eq(row.get<RuntimePosition>().x, 10);
            cr_assert_eq(row.get<RuntimeVelocity>().x, 4);
            cr_assert_eq(row.tryGet<RuntimeSleeping>(), nullptr);

            row.get<RuntimePosition>().x += row.get<RuntimeVelocity>().x;
            positionSum += row.get<RuntimePosition>().x;
        });

    cr_assert_eq(visited, moving);
    cr_assert_eq(positionSum, 14);
    cr_assert_eq(world.get<RuntimePosition>(moving).x, 14);
    cr_assert_eq(world.get<RuntimePosition>(sleeping).x, 50);
}

Test(query, runtime_query_can_be_built_from_explicit_query_description) {
    World world;
    world.registerComponent<RuntimePosition>();
    world.registerComponent<RuntimeVelocity>();
    world.registerComponent<RuntimeSleeping>();

    const Entity awake = world.createEntity();
    const Entity sleeping = world.createEntity();

    world.set(awake, RuntimePosition{.x = 1});
    world.set(awake, RuntimeVelocity{.x = 2});

    world.set(sleeping, RuntimePosition{.x = 3});
    world.set(sleeping, RuntimeVelocity{.x = 4});
    world.add<RuntimeSleeping>(sleeping);

    Query desc;
    desc.with<RuntimePosition>()
        .with<RuntimeVelocity>()
        .without<RuntimeSleeping>();

    uint32_t matched = 0;
    world.query(desc).each([&](const RowView row) {
        matched += 1;
        cr_assert_eq(row.entity(), awake);
    });

    cr_assert_eq(matched, 1);
}

Test(query, runtime_query_is_uncached_and_observes_world_changes_between_calls) {
    World world;
    world.registerComponent<RuntimePosition>();
    world.registerComponent<RuntimeVelocity>();

    const Entity first = world.createEntity();
    world.set(first, RuntimePosition{.x = 1});
    world.set(first, RuntimeVelocity{.x = 2});

    uint32_t firstPass = 0;
    world.query()
        .with<RuntimePosition>()
        .with<RuntimeVelocity>()
        .each([&](const RowView) {
            firstPass += 1;
        });

    const Entity second = world.createEntity();
    world.set(second, RuntimePosition{.x = 3});
    world.set(second, RuntimeVelocity{.x = 4});

    uint32_t secondPass = 0;
    world.query()
        .with<RuntimePosition>()
        .with<RuntimeVelocity>()
        .each([&](const RowView row) {
            secondPass += 1;
            row.get<RuntimePosition>().x += 10;
        });

    cr_assert_eq(firstPass, 1);
    cr_assert_eq(secondPass, 2);
    cr_assert_eq(world.get<RuntimePosition>(first).x, 11);
    cr_assert_eq(world.get<RuntimePosition>(second).x, 13);
}

Test(query, empty_query_has_no_required_components) {
    const Query result = query<>();

    cr_assert_eq(result.required_count, 0);
    cr_assert_eq(result.required[0], UINT16_MAX);
}

Test(query, with_term_populates_required_component_ids_in_order) {
    const Query result = query<Without<NoIntegrate>, With<Position, Velocity>>();

    cr_assert_eq(result.required_count, 2);
    cr_assert_eq(result.required[0], ComponentRegistry::id<Position>());
    cr_assert_eq(result.required[1], ComponentRegistry::id<Velocity>());

    cr_assert_eq(result.excluded_count, 1);
    cr_assert_eq(result.excluded[0], ComponentRegistry::id<NoIntegrate>());
}

Test(query, multiple_with_and_without_terms_are_merged_without_duplicates) {
    const Query result = query<
        With<Position>,
        Without<NoIntegrate>,
        With<Velocity>,
        Without<NoIntegrate>
    >();

    cr_assert_eq(result.required_count, 2);
    cr_assert_eq(result.required[0], ComponentRegistry::id<Position>());
    cr_assert_eq(result.required[1], ComponentRegistry::id<Velocity>());
    cr_assert_eq(result.excluded_count, 1);
    cr_assert_eq(result.excluded[0], ComponentRegistry::id<NoIntegrate>());
}
