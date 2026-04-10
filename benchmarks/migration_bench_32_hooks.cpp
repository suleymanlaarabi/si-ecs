#include "BenchCommon.hpp"
#include "World.hpp"
#include <string>
#include <vector>

static uint64_t hookCount = 0;

#define DEFINE_HOOK_COMPONENT(name, bytes_count) \
    struct name { \
        unsigned char bytes[bytes_count]; \
        static void onAdd(Entity, World&) { \
            hookCount += 1; \
        } \
    }

DEFINE_HOOK_COMPONENT(C01, 4);
DEFINE_HOOK_COMPONENT(C02, 8);
DEFINE_HOOK_COMPONENT(C03, 12);
DEFINE_HOOK_COMPONENT(C04, 16);
DEFINE_HOOK_COMPONENT(C05, 24);
DEFINE_HOOK_COMPONENT(C06, 32);
DEFINE_HOOK_COMPONENT(C07, 48);
DEFINE_HOOK_COMPONENT(C08, 64);
DEFINE_HOOK_COMPONENT(C09, 4);
DEFINE_HOOK_COMPONENT(C10, 8);
DEFINE_HOOK_COMPONENT(C11, 12);
DEFINE_HOOK_COMPONENT(C12, 16);
DEFINE_HOOK_COMPONENT(C13, 24);
DEFINE_HOOK_COMPONENT(C14, 32);
DEFINE_HOOK_COMPONENT(C15, 48);
DEFINE_HOOK_COMPONENT(C16, 64);
DEFINE_HOOK_COMPONENT(C17, 4);
DEFINE_HOOK_COMPONENT(C18, 8);
DEFINE_HOOK_COMPONENT(C19, 12);
DEFINE_HOOK_COMPONENT(C20, 16);
DEFINE_HOOK_COMPONENT(C21, 24);
DEFINE_HOOK_COMPONENT(C22, 32);
DEFINE_HOOK_COMPONENT(C23, 48);
DEFINE_HOOK_COMPONENT(C24, 64);
DEFINE_HOOK_COMPONENT(C25, 4);
DEFINE_HOOK_COMPONENT(C26, 8);
DEFINE_HOOK_COMPONENT(C27, 12);
DEFINE_HOOK_COMPONENT(C28, 16);
DEFINE_HOOK_COMPONENT(C29, 24);
DEFINE_HOOK_COMPONENT(C30, 32);
DEFINE_HOOK_COMPONENT(C31, 48);
DEFINE_HOOK_COMPONENT(C32, 64);

#undef DEFINE_HOOK_COMPONENT

template <typename T>
static void addComponentBatch(World& world, const std::vector<Entity>& entities,
                              const std::string& label, const T& value) {
    const ComponentId cid = ComponentType::id<T>();
    BenchTimer timer(label);
    for (const Entity entity : entities) {
        world.setComponent(entity, cid, &value);
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
    world.registerComponent<C16>();
    world.registerComponent<C17>();
    world.registerComponent<C18>();
    world.registerComponent<C19>();
    world.registerComponent<C20>();
    world.registerComponent<C21>();
    world.registerComponent<C22>();
    world.registerComponent<C23>();
    world.registerComponent<C24>();
    world.registerComponent<C25>();
    world.registerComponent<C26>();
    world.registerComponent<C27>();
    world.registerComponent<C28>();
    world.registerComponent<C29>();
    world.registerComponent<C30>();
    world.registerComponent<C31>();
    world.registerComponent<C32>();

    constexpr int entityCount = 120000;
    std::vector<Entity> entities;
    entities.reserve(entityCount);

    {
        BenchTimer timer("Create Entities");
        for (int i = 0; i < entityCount; ++i) {
            entities.push_back(world.createEntity());
        }
    }

    addComponentBatch(world, entities, "Add C01 (4B, onAdd)", C01{});
    addComponentBatch(world, entities, "Add C02 (8B, onAdd)", C02{});
    addComponentBatch(world, entities, "Add C03 (12B, onAdd)", C03{});
    addComponentBatch(world, entities, "Add C04 (16B, onAdd)", C04{});
    addComponentBatch(world, entities, "Add C05 (24B, onAdd)", C05{});
    addComponentBatch(world, entities, "Add C06 (32B, onAdd)", C06{});
    addComponentBatch(world, entities, "Add C07 (48B, onAdd)", C07{});
    addComponentBatch(world, entities, "Add C08 (64B, onAdd)", C08{});
    addComponentBatch(world, entities, "Add C09 (4B, onAdd)", C09{});
    addComponentBatch(world, entities, "Add C10 (8B, onAdd)", C10{});
    addComponentBatch(world, entities, "Add C11 (12B, onAdd)", C11{});
    addComponentBatch(world, entities, "Add C12 (16B, onAdd)", C12{});
    addComponentBatch(world, entities, "Add C13 (24B, onAdd)", C13{});
    addComponentBatch(world, entities, "Add C14 (32B, onAdd)", C14{});
    addComponentBatch(world, entities, "Add C15 (48B, onAdd)", C15{});
    addComponentBatch(world, entities, "Add C16 (64B, onAdd)", C16{});
    addComponentBatch(world, entities, "Add C17 (4B, onAdd)", C17{});
    addComponentBatch(world, entities, "Add C18 (8B, onAdd)", C18{});
    addComponentBatch(world, entities, "Add C19 (12B, onAdd)", C19{});
    addComponentBatch(world, entities, "Add C20 (16B, onAdd)", C20{});
    addComponentBatch(world, entities, "Add C21 (24B, onAdd)", C21{});
    addComponentBatch(world, entities, "Add C22 (32B, onAdd)", C22{});
    addComponentBatch(world, entities, "Add C23 (48B, onAdd)", C23{});
    addComponentBatch(world, entities, "Add C24 (64B, onAdd)", C24{});
    addComponentBatch(world, entities, "Add C25 (4B, onAdd)", C25{});
    addComponentBatch(world, entities, "Add C26 (8B, onAdd)", C26{});
    addComponentBatch(world, entities, "Add C27 (12B, onAdd)", C27{});
    addComponentBatch(world, entities, "Add C28 (16B, onAdd)", C28{});
    addComponentBatch(world, entities, "Add C29 (24B, onAdd)", C29{});
    addComponentBatch(world, entities, "Add C30 (32B, onAdd)", C30{});
    addComponentBatch(world, entities, "Add C31 (48B, onAdd)", C31{});
    addComponentBatch(world, entities, "Add C32 (64B, onAdd)", C32{});

    return static_cast<int>(hookCount == 0 ? 1 : 0);
}
