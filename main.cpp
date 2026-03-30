#include <iostream>

#include "Addons.hpp"
#include "Commands.hpp"
#include "World.hpp"
#include "System.hpp"

struct CollisionStart {
    static void onAdd(const Entity entity, World& world) {
        puts("added");
        world.remove<CollisionStart>(entity);
    }

    static void onRemove(Entity, World&) {
        puts("removed");
    }
};

struct Position {
    float x, y;

    bool operator==(const Position&) const = default;
};

struct Velocity {
    float x, y;
};

struct NoIntegrate;

SystemDesc PropagatePosition = each([]() {});


SystemDesc VelocitySys = each<Without<NoIntegrate>>(
    [](Position& pos, const Velocity& vel, const DeltaTime& delta) {
        pos.x += vel.x * delta.value;
        pos.y += vel.y * delta.value;
    });


int main() {
    World world;
    world.registerComponent<Position>();

    const auto e = world.createEntity();

    world.add<Position>(e);

    world.system(Phase::Update, each([](Position& pos, Velocity& vel, Commands commands) {
        const Entity entity = commands.spawn();
        commands.add<Position>(entity);
        commands.kill(entity);
        pos.x += vel.x;
        pos.y += vel.x;
    }));


    return 0;
}
