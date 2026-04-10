#include "BenchCommon.hpp"
#include "World.hpp"
#include <array>
#include <string>
#include <vector>

struct C01 { float v[4]; };
struct C02 { float v[4]; };
struct C03 { float v[4]; };
struct C04 { float v[4]; };
struct C05 { float v[4]; };
struct C06 { float v[4]; };
struct C07 { float v[4]; };
struct C08 { float v[4]; };
struct C09 { float v[4]; };
struct C10 { float v[4]; };
struct C11 { float v[4]; };
struct C12 { float v[4]; };
struct C13 { float v[4]; };
struct C14 { float v[4]; };
struct C15 { float v[4]; };

template <typename T>
static void addComponentBatch(World& world, const std::vector<Entity>& entities,
                              const std::string& label, const T& value) {
    const ComponentId cid = ComponentType::id<T>();
    BenchTimer timer(label);
    for (const Entity entity : entities) {
        world.setComponent(entity, cid, &value);
    }
}

template <typename T>
static void removeComponentBatch(World& world, const std::vector<Entity>& entities,
                                 const std::string& label) {
    const ComponentId cid = ComponentType::id<T>();
    BenchTimer timer(label);
    for (const Entity entity : entities) {
        world.removeComponent(entity, cid);
    }
}

int main() {
    World world;
    world.registerComponent<C01>();
    world.registerComponent<C02>();
    world.registerComponent<C03>();
    world.registerComponent<C04>();
    world.registerComponent<C05>();
    world.registerComponent<C06>();
    world.registerComponent<C07>();
    world.registerComponent<C08>();
    world.registerComponent<C09>();
    world.registerComponent<C10>();
    world.registerComponent<C11>();
    world.registerComponent<C12>();
    world.registerComponent<C13>();
    world.registerComponent<C14>();
    world.registerComponent<C15>();

    constexpr int entityCount = 300000;
    std::vector<Entity> entities;
    entities.reserve(entityCount);

    {
        BenchTimer timer("Create Entities");
        for (int i = 0; i < entityCount; ++i) {
            entities.push_back(world.createEntity());
        }
    }

    addComponentBatch(world, entities, "Add C01 (No Migration)", C01{{1.f, 2.f, 3.f, 4.f}});
    addComponentBatch(world, entities, "Add C02", C02{{2.f, 3.f, 4.f, 5.f}});
    addComponentBatch(world, entities, "Add C03", C03{{3.f, 4.f, 5.f, 6.f}});
    addComponentBatch(world, entities, "Add C04", C04{{4.f, 5.f, 6.f, 7.f}});
    addComponentBatch(world, entities, "Add C05", C05{{5.f, 6.f, 7.f, 8.f}});
    addComponentBatch(world, entities, "Add C06", C06{{6.f, 7.f, 8.f, 9.f}});
    addComponentBatch(world, entities, "Add C07", C07{{7.f, 8.f, 9.f, 10.f}});
    addComponentBatch(world, entities, "Add C08", C08{{8.f, 9.f, 10.f, 11.f}});
    addComponentBatch(world, entities, "Add C09", C09{{9.f, 10.f, 11.f, 12.f}});
    addComponentBatch(world, entities, "Add C10", C10{{10.f, 11.f, 12.f, 13.f}});
    addComponentBatch(world, entities, "Add C11", C11{{11.f, 12.f, 13.f, 14.f}});
    addComponentBatch(world, entities, "Add C12", C12{{12.f, 13.f, 14.f, 15.f}});
    addComponentBatch(world, entities, "Add C13", C13{{13.f, 14.f, 15.f, 16.f}});
    addComponentBatch(world, entities, "Add C14", C14{{14.f, 15.f, 16.f, 17.f}});
    addComponentBatch(world, entities, "Add C15", C15{{15.f, 16.f, 17.f, 18.f}});

    removeComponentBatch<C08>(world, entities, "Remove C08");

    return 0;
}
