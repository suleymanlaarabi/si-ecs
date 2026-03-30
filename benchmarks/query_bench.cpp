#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>

#include "BenchCommon.hpp"

namespace {
    struct QueryBenchConfig {
        uint32_t samples = 12;
        uint32_t registeredComponents = 60;
        uint32_t tableCount = 10000;
        uint32_t entitiesPerTable = 24;
        uint32_t minComponentsPerTable = 8;
        uint32_t maxComponentsPerTable = 60;
        uint32_t queryRequired = 8;
        uint32_t queryExcluded = 4;
    };

    struct QueryBenchState {
        std::unique_ptr<World> world;
        Query query;
    };

    uint64_t mix64(uint64_t value) {
        value ^= value >> 30;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27;
        value *= 0x94d049bb133111ebULL;
        value ^= value >> 31;
        return value;
    }

    template <size_t... I>
    void registerBenchComponentsUpTo(World& world, const uint32_t count, std::index_sequence<I...>) {
        ((I < bench::BenchComponentCount && I < count
              ? (world.registerComponent<bench::BenchComponent<I>>(), void())
              : void()), ...);
    }

    template <size_t... I>
    void applyMask(World& world, const Entity entity, const std::array<uint64_t, 4>& words, std::index_sequence<I...>) {
        ComponentId components[bench::BenchComponentCount];
        uint32_t count = 0;

        (((I < bench::BenchComponentCount) && ((words[I / 64] & (uint64_t{1} << (I % 64))) != 0)
              ? (components[count++] = ComponentRegistry::id<bench::BenchComponent<I>>(), void())
              : void()), ...);

        world.addBatchComponents(entity, components, count);

        (((I < bench::BenchComponentCount) && ((words[I / 64] & (uint64_t{1} << (I % 64))) != 0)
              ? (world.get<bench::BenchComponent<I>>(entity).value = static_cast<uint32_t>(entity.index + I), void())
              : void()), ...);
    }

    std::array<uint64_t, 4> buildArchetypeMask(const QueryBenchConfig& config, const uint32_t archetypeIndex) {
        std::array<uint64_t, 4> words = {};
        const uint32_t range = config.maxComponentsPerTable - config.minComponentsPerTable + 1;
        const uint32_t componentCount = config.minComponentsPerTable + (mix64(archetypeIndex + 1) % range);

        uint32_t added = 0;
        uint64_t seed = mix64(archetypeIndex + 0x9e3779b97f4a7c15ULL);
        while (added < componentCount) {
            const uint32_t component = static_cast<uint32_t>(seed % config.registeredComponents);
            const uint64_t bit = uint64_t{1} << (component % 64);
            uint64_t& word = words[component / 64];
            if ((word & bit) == 0) {
                word |= bit;
                added += 1;
            }
            seed = mix64(seed + added + 1);
        }

        return words;
    }

    Query makeConfiguredQuery(const QueryBenchConfig& config) {
        Query query;

        for (uint32_t i = 0; i < config.queryRequired; ++i) {
            query.with(static_cast<ComponentId>(i));
        }

        for (uint32_t i = 0; i < config.queryExcluded; ++i) {
            query.without(static_cast<ComponentId>(config.queryRequired + i));
        }

        return query;
    }

    void prepareWorld(World& world, const QueryBenchConfig& config) {
        registerBenchComponentsUpTo(world, config.registeredComponents, bench::BenchComponentIndex{});

        for (uint32_t tableIndex = 0; tableIndex < config.tableCount; ++tableIndex) {
            const auto mask = buildArchetypeMask(config, tableIndex);
            for (uint32_t row = 0; row < config.entitiesPerTable; ++row) {
                const Entity entity = world.createEntity();
                applyMask(world, entity, mask, bench::BenchComponentIndex{});
            }
        }
    }

    void printUsage() {
        std::cout
            << "usage: ecs_bench_query [--samples N] [--registered-components N] [--tables N] "
            << "[--entities-per-table N] [--min-components N] [--max-components N] "
            << "[--query-required N] [--query-excluded N]\n";
    }

    bool parseArgs(const int argc, char** argv, QueryBenchConfig& config) {
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
            } else if (arg == "--registered-components") {
                config.registeredComponents = value;
            } else if (arg == "--tables") {
                config.tableCount = value;
            } else if (arg == "--entities-per-table") {
                config.entitiesPerTable = value;
            } else if (arg == "--min-components") {
                config.minComponentsPerTable = value;
            } else if (arg == "--max-components") {
                config.maxComponentsPerTable = value;
            } else if (arg == "--query-required") {
                config.queryRequired = value;
            } else if (arg == "--query-excluded") {
                config.queryExcluded = value;
            } else {
                std::cerr << "unknown argument: " << arg << '\n';
                return false;
            }

            i += 1;
        }

        config.registeredComponents = std::clamp<uint32_t>(config.registeredComponents, 1, bench::BenchComponentCount);
        config.tableCount = std::max<uint32_t>(config.tableCount, 1);
        config.entitiesPerTable = std::max<uint32_t>(config.entitiesPerTable, 1);
        config.minComponentsPerTable = std::clamp<uint32_t>(config.minComponentsPerTable, 1,
                                                            config.registeredComponents);
        config.maxComponentsPerTable = std::clamp<uint32_t>(config.maxComponentsPerTable,
                                                            config.minComponentsPerTable,
                                                            config.registeredComponents);
        config.queryRequired = std::min<uint32_t>(config.queryRequired, MAX_QUERY_TERM);
        config.queryExcluded = std::min<uint32_t>(config.queryExcluded, MAX_QUERY_TERM - config.queryRequired);

        return true;
    }
}

int main(const int argc, char** argv) {
    QueryBenchConfig config;
    if (!parseArgs(argc, argv, config)) {
        return argc > 1 ? 1 : 0;
    }

    const uint32_t entityCount = config.tableCount * config.entitiesPerTable;
    const Query query = makeConfiguredQuery(config);

    bench::printHeader("query_bench");
    bench::printConfigValue("samples", config.samples);
    bench::printConfigValue("registered_components", config.registeredComponents);
    bench::printConfigValue("tables", config.tableCount);
    bench::printConfigValue("entities_per_table", config.entitiesPerTable);
    bench::printConfigValue("min_components_per_table", config.minComponentsPerTable);
    bench::printConfigValue("max_components_per_table", config.maxComponentsPerTable);
    bench::printConfigValue("query_required", config.queryRequired);
    bench::printConfigValue("query_excluded", config.queryExcluded);

    bench::printResult(bench::runPrepared(
        "register_cached_query",
        config.samples,
        1,
        [&]() -> QueryBenchState {
            auto world = std::make_unique<World>();
            prepareWorld(*world, config);
            return {
                .world = std::move(world),
                .query = query
            };
        },
        [](QueryBenchState& state) -> uint64_t {
            const QueryId qid = state.world->cacheQuery(state.query);
            return state.world->getQueries()[qid].tables.size;
        }
    ));

    bench::printResult(bench::runPrepared(
        "execute_uncached_runtime_query",
        config.samples,
        entityCount,
        [&]() -> QueryBenchState {
            auto world = std::make_unique<World>();
            prepareWorld(*world, config);
            return {
                .world = std::move(world),
                .query = query
            };
        },
        [](QueryBenchState& state) -> uint64_t {
            uint64_t matched = 0;
            state.world->query(state.query).each([&](const RowView row) {
                matched += row.entity().index;
            });
            return matched;
        }
    ));

    return 0;
}
