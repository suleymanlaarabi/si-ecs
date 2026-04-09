#pragma once
#include <utility>
#include <vector>

#include "EcsAssert.hpp"
#include "Table.hpp"

static __attribute__((always_inline)) uint64_t inline mix64(uint64_t x) noexcept {
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ull;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBull;
    x ^= x >> 31;
    return x;
}

static __attribute__((always_inline)) uint64_t inline hashEntityType(const EntityType &e) noexcept {
    const auto *data = reinterpret_cast<const uint8_t *>(e.cids);
    const size_t len = static_cast<size_t>(e.count) * sizeof(ComponentId);

    uint64_t h = mix64(0x9E3779B97F4A7C15ull ^ static_cast<uint64_t>(e.count));

    size_t i = 0;
    for (; i + sizeof(uint64_t) <= len; i += sizeof(uint64_t)) {
        uint64_t block;
        std::memcpy(&block, data + i, sizeof(block));
        h ^= mix64(block + 0x9E3779B97F4A7C15ull);
        h *= 0x9E3779B97F4A7C15ull;
    }

    if (i < len) {
        uint64_t tail = 0;
        std::memcpy(&tail, data + i, len - i);
        h ^= mix64(tail + 0xD6E8FEB86659FD93ull);
        h *= 0x9E3779B97F4A7C15ull;
    }

    h ^= mix64(len);
    return mix64(h);
}

struct Bucket {
    TableId tableId = InvalidTableId;
    uint64_t hash = UINT64_MAX;
};

struct TableMap {
    std::vector<Table> tables;
    Bucket *buckets = nullptr;
    uint32_t size = 0;
    uint32_t count = 0;

    inline void eraseBucketAt(const uint32_t bucketIndex) {
        const uint32_t mask = size - 1u;
        buckets[bucketIndex] = {};
        count -= 1;

        uint32_t idx = (bucketIndex + 1) & mask;
        while (buckets[idx].tableId != InvalidTableId) {
            const Bucket moved = buckets[idx];
            buckets[idx] = {};

            uint32_t newIdx = moved.hash & mask;
            while (buckets[newIdx].tableId != InvalidTableId) {
                newIdx = (newIdx + 1) & mask;
            }

            buckets[newIdx] = moved;
            idx = (idx + 1) & mask;
        }
    }

    inline void growBuckets() {
        const uint32_t newSize = size * 2;
        auto *newBuckets = new Bucket[newSize];
        const auto newMask = newSize - 1u;

        for (uint32_t i = 0; i < size; i++) {
            const Bucket &oldBucket = buckets[i];
            if (oldBucket.tableId == InvalidTableId) {
                continue;
            }
            uint32_t newIdx = oldBucket.hash & newMask;

            while (true) {
                const Bucket &b = newBuckets[newIdx];

                if (b.tableId == InvalidTableId) {
                    break;
                }

                newIdx = (newIdx + 1) & newMask;
            }

            newBuckets[newIdx] = oldBucket;
        }

        delete[] buckets;
        buckets = newBuckets;
        size = newSize;
    }

    [[nodiscard]] inline uint32_t findBucketIndex(const EntityType &e, const uint64_t hash) const {
        const uint32_t mask = size - 1u;
        uint32_t idx = hash & mask;

        while (true) {
            const Bucket &b = buckets[idx];
            if (b.tableId == InvalidTableId || (b.hash == hash && e == tables[b.tableId].getType())) {
                return idx;
            }
            idx = (idx + 1) & mask;
        }
    }

    [[nodiscard]] inline std::pair<TableId, Table &> createAt(const uint32_t idx, const uint64_t hash,
                                                              EntityType &&ownedType,
                                                              ComponentRegistry &componentRegistry,
                                                              bool &isCreated) {
        Bucket &bucket = buckets[idx];
        ecs_assert(tables.size() <= UINT16_MAX, "table id limit reached");
        const auto tid = static_cast<TableId>(tables.size());
        Table &table = tables.emplace_back(std::move(ownedType), componentRegistry, tid);
        bucket.tableId = tid;
        bucket.hash = hash;
        count += 1;
        isCreated = true;
        return {tid, table};
    }

public:
    TableMap() {
        this->size = 16;
        this->count = 0;
        this->buckets = new Bucket[16];
    }

    TableMap(const TableMap &) = delete;

    TableMap &operator=(const TableMap &) = delete;

    ~TableMap() {
        delete[] this->buckets;
    }


    [[nodiscard]] inline bool contains(const EntityType &e) const {
        const uint64_t hash = hashEntityType(e);
        return buckets[findBucketIndex(e, hash)].tableId != InvalidTableId;
    }

    [[nodiscard]] inline std::pair<TableId, Table &> findOrCreate(EntityType &&e,
                                                                  ComponentRegistry &componentRegistry,
                                                                  bool &isCreated) {
        if ((count + 1) * 2 > size) {
            growBuckets();
        }

        const uint64_t hash = hashEntityType(e);
        const uint32_t idx = findBucketIndex(e, hash);
        if (const Bucket &bucket = buckets[idx]; bucket.tableId != InvalidTableId) {
            e.release();
            e = {};
            return {bucket.tableId, tables[bucket.tableId]};
        }

        return createAt(idx, hash, std::move(e), componentRegistry, isCreated);
    }
};
