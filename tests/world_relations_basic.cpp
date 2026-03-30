#include <criterion/criterion.h>

#include "../ecs/EcsType.hpp"
#include "../ecs/World.hpp"


Test(world, relate_creates_target_and_source_components) {
    World world;

    const Entity parent = world.createEntity();
    const Entity child = world.createEntity();

    world.relate<ChildOf>(child, parent);

    cr_assert(world.hasTarget<ChildOf>(child, parent));
    cr_assert(world.hasSource<ChildOf>(parent, child));
}

Test(world, relate_updates_previous_source_when_target_changes) {
    World world;

    const Entity firstParent = world.createEntity();
    const Entity secondParent = world.createEntity();
    const Entity child = world.createEntity();

    world.relate<ChildOf>(child, firstParent);
    world.relate<ChildOf>(child, secondParent);

    cr_assert(world.hasTarget<ChildOf>(child, secondParent));
    cr_assert_not(world.hasTarget<ChildOf>(child, firstParent));
    cr_assert(world.hasSource<ChildOf>(secondParent, child));
    cr_assert_not(world.hasSource<ChildOf>(firstParent, child));
}

Test(world, relate_keeps_old_source_while_other_children_still_point_to_it) {
    World world;

    const Entity firstParent = world.createEntity();
    const Entity secondParent = world.createEntity();
    const Entity firstChild = world.createEntity();
    const Entity secondChild = world.createEntity();

    world.relate<ChildOf>(firstChild, firstParent);
    world.relate<ChildOf>(secondChild, firstParent);
    world.relate<ChildOf>(firstChild, secondParent);

    cr_assert(world.hasSource<ChildOf>(firstParent, secondChild));
    cr_assert_not(world.hasSource<ChildOf>(firstParent, firstChild));
    cr_assert(world.hasSource<ChildOf>(secondParent, firstChild));
}

Test(world, unrelate_removes_target_and_empty_source) {
    World world;

    const Entity parent = world.createEntity();
    const Entity child = world.createEntity();

    world.relate<ChildOf>(child, parent);
    world.unrelate<ChildOf>(child, parent);

    cr_assert_not(world.has<RelationTarget<ChildOf>>(child));
    cr_assert_not(world.hasTarget<ChildOf>(child, parent));
    cr_assert_not(world.hasSource<ChildOf>(parent, child));
    cr_assert_not(world.has<RelationSource<ChildOf>>(parent));
}

Test(world, unrelate_keeps_source_when_other_children_still_reference_target) {
    World world;

    const Entity parent = world.createEntity();
    const Entity firstChild = world.createEntity();
    const Entity secondChild = world.createEntity();

    world.relate<ChildOf>(firstChild, parent);
    world.relate<ChildOf>(secondChild, parent);
    world.unrelate<ChildOf>(firstChild, parent);

    cr_assert_not(world.has<RelationTarget<ChildOf>>(firstChild));
    cr_assert_not(world.hasSource<ChildOf>(parent, firstChild));
    cr_assert(world.hasSource<ChildOf>(parent, secondChild));
    cr_assert(world.has<RelationSource<ChildOf>>(parent));
}

Test(world, destroy_entity_removes_relation_source_membership) {
    World world;

    const Entity parent = world.createEntity();
    const Entity child = world.createEntity();

    world.relate<ChildOf>(child, parent);
    world.destroyEntity(child);

    cr_assert_not(world.isAlive(child));
    cr_assert_not(world.hasSource<ChildOf>(parent, child));
    cr_assert_not(world.has<RelationSource<ChildOf>>(parent));
}
