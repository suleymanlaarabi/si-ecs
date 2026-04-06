
#include "Addons.hpp"
#include "Commands.hpp"
#include "Query.hpp"
#include "SystemRegistry.hpp"
#include "World.hpp"
#include "System.hpp"

struct CollisionStart {
    static void onAdd(const Entity entity, World &world) {
        puts("added");
        world.remove<CollisionStart>(entity);
    }

    static void onRemove(Entity, World &) {
        puts("removed");
    }
};

struct Position {
    float x, y;

    bool operator==(const Position &) const = default;
};

struct Velocity {
    float x, y;
};

struct NoIntegrate;


static inline
void VelocitySys(Position &pos, const Velocity &vel) {
    pos.x += vel.x;
    pos.y += vel.y;
}

int main() {
    World world;
    world.registerComponent<Position>();

    const auto e = world.createEntity();

    world.add<Position>(e);


    world.system(Phase::Update, each<VelocitySys, Without<NoIntegrate>>());


    return 0;
}
