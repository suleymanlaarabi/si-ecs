#include "World.hpp"
#include "BenchCommon.hpp"
#include <vector>

struct Pos { float x, y, z; };
struct Vel { float x, y, z; };
struct Acc { float x, y, z; };

int main() {
    World world;
    world.registerComponent<Pos>();
    world.registerComponent<Vel>();
    world.registerComponent<Acc>();

    const ComponentId pos_id = ComponentType::id<Pos>();
    const ComponentId vel_id = ComponentType::id<Vel>();
    const ComponentId acc_id = ComponentType::id<Acc>();

    const int ENTITY_COUNT = 1000000;
    std::vector<Entity> entities;
    entities.reserve(ENTITY_COUNT);

    {
        BenchTimer timer("Create Entities");
        for (int i = 0; i < ENTITY_COUNT; ++i) {
            entities.push_back(world.createEntity());
        }
    }

    {
        BenchTimer timer("Add Pos (No Migration)");
        for (auto e : entities) {
            Pos p = {1.0f, 2.0f, 3.0f};
            world.setComponent(e, pos_id, &p);
        }
    }

    {
        BenchTimer timer("Add Vel (Migrate [Pos] -> [Pos, Vel])");
        for (auto e : entities) {
            Vel v = {0.1f, 0.2f, 0.3f};
            world.setComponent(e, vel_id, &v);
        }
    }

    {
        BenchTimer timer("Add Acc (Migrate [Pos, Vel] -> [Pos, Vel, Acc])");
        for (auto e : entities) {
            Acc a = {0.01f, 0.02f, 0.03f};
            world.setComponent(e, acc_id, &a);
        }
    }

    {
        BenchTimer timer("Remove Vel (Migrate [Pos, Vel, Acc] -> [Pos, Acc])");
        for (auto e : entities) {
            world.removeComponent(e, vel_id);
        }
    }

    return 0;
}
