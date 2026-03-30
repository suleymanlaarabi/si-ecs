#pragma once

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>

#include "ecs/World.hpp"

namespace bench {
using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

inline volatile uint64_t sink = 0;

struct Result {
    const char* name;
    double total_ms;
};

template <typename Func>
Result run(const char* name, const uint64_t samples, const uint64_t operationsPerSample, Func&& func) {
    Nanoseconds total{0};

    for (uint64_t i = 0; i < samples; ++i) {
        const auto start = Clock::now();
        sink ^= static_cast<uint64_t>(func());
        const auto end = Clock::now();
        total += std::chrono::duration_cast<Nanoseconds>(end - start);
    }

    const double totalMs = static_cast<double>(total.count()) / 1'000'000.0;
    (void)operationsPerSample;
    return {
        .name = name,
        .total_ms = totalMs
    };
}

template <typename Setup, typename Func>
Result runPrepared(const char* name, const uint64_t samples, const uint64_t operationsPerSample,
                   Setup&& setup, Func&& func) {
    Nanoseconds total{0};

    for (uint64_t i = 0; i < samples; ++i) {
        auto state = setup();
        const auto start = Clock::now();
        sink ^= static_cast<uint64_t>(func(state));
        const auto end = Clock::now();
        total += std::chrono::duration_cast<Nanoseconds>(end - start);
    }

    (void)operationsPerSample;
    return {
        .name = name,
        .total_ms = static_cast<double>(total.count()) / 1'000'000.0
    };
}

inline void printHeader(const char* suite) {
    std::cout << suite << '\n';
}

inline void printConfigValue(const std::string_view key, const uint32_t value) {
    std::cout << "  " << key << ": " << value << '\n';
}

inline void printResult(const Result& result) {
    constexpr int NameWidth = 48;
    std::cout << "  "
              << std::left << std::setw(NameWidth) << std::string_view(result.name)
              << " : "
              << std::right << std::fixed << std::setprecision(3) << result.total_ms
              << " ms\n";
}

inline bool parseUintArg(const char* value, uint32_t& out) {
    const auto parsed = std::strtoull(value, nullptr, 10);
    if (parsed > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    out = static_cast<uint32_t>(parsed);
    return true;
}

template <size_t I>
struct BenchComponent {
    uint32_t value = static_cast<uint32_t>(I);
};

constexpr size_t BenchComponentCount = 64;
using BenchComponentIndex = std::make_index_sequence<BenchComponentCount>;

template <size_t... I>
void registerBenchComponents(World& world, std::index_sequence<I...>) {
    (world.registerComponent<BenchComponent<I>>(), ...);
}

template <size_t... I>
void setBenchComponents(World& world, const Entity entity, std::index_sequence<I...>) {
    (world.set<BenchComponent<I>>(entity, BenchComponent<I>{static_cast<uint32_t>(entity.index + I)}), ...);
}

template <size_t... I>
void removeBenchComponents(World& world, const Entity entity, std::index_sequence<I...>) {
    (world.remove<BenchComponent<I>>(entity), ...);
}
}
