#pragma once
#include <cstdint>
#include "EcsType.hpp"


using BloomFilter = uint64_t;

inline BloomFilter bloomFromEntityType(const EntityType& e) {
    BloomFilter filter = 0;

    for (uint16_t i = 0; i < e.count; i++) {
        filter |= (1ull << (e.cids[i] % 64));
    }

    return filter;
}

