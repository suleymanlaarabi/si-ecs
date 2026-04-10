
#include <iostream>

#include "Addons.hpp"
#include "Commands.hpp"
#include "Query.hpp"
#include "SystemRegistry.hpp"
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

struct ToggleComponent {
    static constexpr bool isToggleComponent = true;
};

struct IsVisible : ToggleComponent {};

static inline
void VelocitySys(Position& pos, const Velocity& vel, DeltaTime delta) {
    pos.x += vel.x;
    pos.y += vel.y;
}

template <typename T>
concept IsToggleComponent = T::isToggleComponent && ecs_sizeof<T>() == 0;


int main() {
    World world;
    std::cout << "ok" << std::endl;
    world.registerComponent<Position>();

    world.system(Phase::Update, task<[](const DeltaTime time) {
        std::cout << time.delta << std::endl;
    }>());

    // just once par frame
    world.system(Phase::Update, task<[](DeltaTime time) {
        time.delta = 10;
    }>());

    // for each entity that match component and filter
    world.onUpdate<[](Position& pos, const Velocity& vel, DeltaTime time) {
        pos.x += vel.x * time.delta;
        pos.y += vel.y * time.delta;
    }, Without<NoIntegrate>>();

    world.system(Phase::Update, each<[](Position& pos, const Velocity& vel) {
        pos.x += vel.x;
        pos.y += vel.y;
    }, Without<NoIntegrate>>());


    world.progress();


    return 0;
}
