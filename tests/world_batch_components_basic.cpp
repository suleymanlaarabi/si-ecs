#include <criterion/criterion.h>

#include "ecs/core/component/ComponentRegistry.hpp"
#include "ecs/core/base/EcsType.hpp"
#include "ecs/World.hpp"

namespace {
    struct BatchTracked {
        static inline int onAddCount = 0;

        static void onAdd(Entity, World&) {
            onAddCount += 1;
        }
    };

    struct BatchExisting {
        int value = 0;
    };

    struct BatchRequiredTracked {
        static inline int onAddCount = 0;

        static void onAdd(Entity, World&) {
            onAddCount += 1;
        }
    };

    struct BatchNeedsTracked : Required<BatchRequiredTracked> {};

    struct BatchAlsoNeedsTracked : Required<BatchRequiredTracked> {};

    struct SingleRequiredTracked {
        static inline int onAddCount = 0;

        static void onAdd(Entity, World&) {
            onAddCount += 1;
        }
    };

    struct SingleNeedsTracked : Required<SingleRequiredTracked> {};

    struct SingleSharedTracked {
        static inline int onAddCount = 0;

        static void onAdd(Entity, World&) {
            onAddCount += 1;
        }
    };

    struct SingleMidA : Required<SingleSharedTracked> {
        static inline int onAddCount = 0;

        static void onAdd(Entity, World&) {
            onAddCount += 1;
        }
    };

    struct SingleMidB : Required<SingleSharedTracked> {
        static inline int onAddCount = 0;

        static void onAdd(Entity, World&) {
            onAddCount += 1;
        }
    };

    struct SingleTop : Required<SingleMidA, SingleMidB> {
        static inline int onAddCount = 0;

        static void onAdd(Entity, World&) {
            onAddCount += 1;
        }
    };
}

Test(world_batch_components, add_batch_components_ignores_duplicates_and_existing_components_for_on_add) {
    World world;
    world.registerComponent<BatchTracked>();
    world.registerComponent<BatchExisting>();

    BatchTracked::onAddCount = 0;

    const Entity entity = world.createEntity();
    world.add<BatchExisting>(entity);

    const ComponentId trackedId = ComponentRegistry::id<BatchTracked>();
    const ComponentId existingId = ComponentRegistry::id<BatchExisting>();
    const ComponentId batch[] = {trackedId, trackedId, existingId};

    world.addBatchComponents(entity, batch, 3);

    cr_assert(world.has<BatchTracked>(entity));
    cr_assert(world.has<BatchExisting>(entity));
    cr_assert_eq(BatchTracked::onAddCount, 1);
}

Test(world_batch_components, add_batch_components_invokes_on_add_for_required_components) {
    World world;
    world.registerComponent<BatchRequiredTracked>();
    world.registerComponent<BatchNeedsTracked>();

    BatchRequiredTracked::onAddCount = 0;

    const Entity entity = world.createEntity();
    const ComponentId batch[] = {ComponentRegistry::id<BatchNeedsTracked>()};

    world.addBatchComponents(entity, batch, 1);

    cr_assert(world.has<BatchNeedsTracked>(entity));
    cr_assert(world.has<BatchRequiredTracked>(entity));
    cr_assert_eq(BatchRequiredTracked::onAddCount, 1);
}

Test(world_batch_components, add_batch_components_deduplicates_required_on_add_hooks) {
    World world;
    world.registerComponent<BatchRequiredTracked>();
    world.registerComponent<BatchNeedsTracked>();
    world.registerComponent<BatchAlsoNeedsTracked>();

    BatchRequiredTracked::onAddCount = 0;

    const Entity entity = world.createEntity();
    const ComponentId batch[] = {
        ComponentRegistry::id<BatchNeedsTracked>(),
        ComponentRegistry::id<BatchAlsoNeedsTracked>()
    };

    world.addBatchComponents(entity, batch, 2);

    cr_assert(world.has<BatchNeedsTracked>(entity));
    cr_assert(world.has<BatchAlsoNeedsTracked>(entity));
    cr_assert(world.has<BatchRequiredTracked>(entity));
    cr_assert_eq(BatchRequiredTracked::onAddCount, 1);
}

Test(world_batch_components, add_component_invokes_on_add_for_required_components) {
    World world;
    world.registerComponent<SingleRequiredTracked>();
    world.registerComponent<SingleNeedsTracked>();

    SingleRequiredTracked::onAddCount = 0;

    const Entity entity = world.createEntity();
    world.add<SingleNeedsTracked>(entity);

    cr_assert(world.has<SingleNeedsTracked>(entity));
    cr_assert(world.has<SingleRequiredTracked>(entity));
    cr_assert_eq(SingleRequiredTracked::onAddCount, 1);
}

Test(world_batch_components, add_component_deduplicates_transitive_required_on_add_hooks) {
    World world;
    world.registerComponent<SingleSharedTracked>();
    world.registerComponent<SingleMidA>();
    world.registerComponent<SingleMidB>();
    world.registerComponent<SingleTop>();

    SingleSharedTracked::onAddCount = 0;
    SingleMidA::onAddCount = 0;
    SingleMidB::onAddCount = 0;
    SingleTop::onAddCount = 0;

    const Entity entity = world.createEntity();
    world.add<SingleTop>(entity);

    cr_assert(world.has<SingleTop>(entity));
    cr_assert(world.has<SingleMidA>(entity));
    cr_assert(world.has<SingleMidB>(entity));
    cr_assert(world.has<SingleSharedTracked>(entity));
    cr_assert_eq(SingleTop::onAddCount, 1);
    cr_assert_eq(SingleMidA::onAddCount, 1);
    cr_assert_eq(SingleMidB::onAddCount, 1);
    cr_assert_eq(SingleSharedTracked::onAddCount, 1);
}
