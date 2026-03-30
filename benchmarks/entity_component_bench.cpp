#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

#include "BenchCommon.hpp"

namespace {
struct EntityBenchConfig {
    uint32_t samples = 8;
    uint32_t entityCount = 1024;
    uint32_t componentCount = 64;
};

template <size_t... I>
void registerBenchComponentsUpTo(World& world, const uint32_t count, std::index_sequence<I...>) {
    ((I < bench::BenchComponentCount && I < count ? (world.registerComponent<bench::BenchComponent<I>>(), void()) : void()), ...);
}

template <size_t... I>
void setBenchComponentsUpTo(World& world, const Entity entity, const uint32_t count, std::index_sequence<I...>) {
    ((I < bench::BenchComponentCount && I < count
          ? (world.set<bench::BenchComponent<I>>(entity, bench::BenchComponent<I>{static_cast<uint32_t>(entity.index + I)}), void())
          : void()), ...);
}

template <size_t... I>
void removeBenchComponentsUpTo(World& world, const Entity entity, const uint32_t count, std::index_sequence<I...>) {
    ((I < bench::BenchComponentCount && I < count ? (world.remove<bench::BenchComponent<I>>(entity), void()) : void()), ...);
}

void prepareWorld(World& world, const EntityBenchConfig& config) {
    registerBenchComponentsUpTo(world, config.componentCount, bench::BenchComponentIndex{});
}

std::vector<Entity> createEmptyEntities(World& world, const uint32_t count) {
    std::vector<Entity> entities;
    entities.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        entities.push_back(world.createEntity());
    }
    return entities;
}

std::vector<Entity> createPopulatedEntities(World& world, const EntityBenchConfig& config) {
    auto entities = createEmptyEntities(world, config.entityCount);
    for (const Entity entity : entities) {
        setBenchComponentsUpTo(world, entity, config.componentCount, bench::BenchComponentIndex{});
    }
    return entities;
}

void printUsage() {
    std::cout << "usage: ecs_bench_entity_component [--samples N] [--entities N] [--components N]\n";
}

bool parseArgs(const int argc, char** argv, EntityBenchConfig& config) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--help") {
            printUsage();
            return false;
        }

        if (i + 1 >= argc) {
            std::cerr << "missing value for " << arg << '\n';
            return false;
        }

        uint32_t value = 0;
        if (!bench::parseUintArg(argv[i + 1], value)) {
            std::cerr << "invalid value for " << arg << '\n';
            return false;
        }

        if (arg == "--samples") {
            config.samples = value;
        } else if (arg == "--entities") {
            config.entityCount = value;
        } else if (arg == "--components") {
            config.componentCount = value;
        } else {
            std::cerr << "unknown argument: " << arg << '\n';
            return false;
        }

        i += 1;
    }

    config.samples = std::max<uint32_t>(config.samples, 1);
    config.entityCount = std::max<uint32_t>(config.entityCount, 1);
    config.componentCount = std::clamp<uint32_t>(config.componentCount, 1, bench::BenchComponentCount);
    return true;
}
}

int main(const int argc, char** argv) {
    EntityBenchConfig config;
    if (!parseArgs(argc, argv, config)) {
        return argc > 1 ? 1 : 0;
    }

    bench::printHeader("entity_component_bench");
    bench::printConfigValue("samples", config.samples);
    bench::printConfigValue("entities", config.entityCount);
    bench::printConfigValue("components", config.componentCount);

    bench::printResult(bench::run(
        "add_components_to_existing_entities",
        config.samples,
        static_cast<uint64_t>(config.entityCount) * config.componentCount,
        [&]() -> uint64_t {
            World world;
            prepareWorld(world, config);
            const auto entities = createEmptyEntities(world, config.entityCount);

            for (const Entity entity : entities) {
                setBenchComponentsUpTo(world, entity, config.componentCount, bench::BenchComponentIndex{});
            }

            return static_cast<uint64_t>(world.getTables().size());
        }
    ));

    bench::printResult(bench::run(
        "remove_components_from_populated_entities",
        config.samples,
        static_cast<uint64_t>(config.entityCount) * config.componentCount,
        [&]() -> uint64_t {
            World world;
            prepareWorld(world, config);
            const auto entities = createPopulatedEntities(world, config);

            for (const Entity entity : entities) {
                removeBenchComponentsUpTo(world, entity, config.componentCount, bench::BenchComponentIndex{});
            }

            return static_cast<uint64_t>(world.getTables().size());
        }
    ));

    bench::printResult(bench::run(
        "create_entities_with_components",
        config.samples,
        config.entityCount,
        [&]() -> uint64_t {
            World world;
            prepareWorld(world, config);
            for (uint32_t i = 0; i < config.entityCount; ++i) {
                const Entity entity = world.createEntity();
                setBenchComponentsUpTo(world, entity, config.componentCount, bench::BenchComponentIndex{});
            }

            return static_cast<uint64_t>(world.getTables().size() + config.entityCount);
        }
    ));

    bench::printResult(bench::run(
        "destroy_entities_with_components",
        config.samples,
        config.entityCount,
        [&]() -> uint64_t {
            World world;
            prepareWorld(world, config);
            const auto entities = createPopulatedEntities(world, config);

            for (const Entity entity : entities) {
                world.destroyEntity(entity);
            }

            return static_cast<uint64_t>(world.getTables().size());
        }
    ));

    return 0;
}
