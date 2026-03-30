#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>

class SparseIndices {
    uint16_t* indices = nullptr;
    uint16_t min = 0;
    uint16_t max = 0;

public:
    SparseIndices() = default;

    SparseIndices(const uint16_t min, const uint16_t max) {
        this->grow(min, max);
    };

    SparseIndices(const SparseIndices&) = delete;
    SparseIndices& operator=(const SparseIndices&) = delete;

    SparseIndices(SparseIndices&& other) noexcept {
        this->indices = other.indices;
        this->min = other.min;
        this->max = other.max;
        other.indices = nullptr;
        other.min = 0;
        other.max = 0;
    }

    SparseIndices& operator=(SparseIndices&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        delete[] this->indices;
        this->indices = other.indices;
        this->min = other.min;
        this->max = other.max;
        other.indices = nullptr;
        other.min = 0;
        other.max = 0;
        return *this;
    }

    ~SparseIndices() {
        delete[] this->indices;
    }

    [[nodiscard]] uint16_t at(const uint16_t id) const {
        if (!this->indices || id < min || id > max) {
            return UINT16_MAX;
        }
        return this->indices[id - min];
    }

    [[nodiscard]] bool has(const uint16_t id) const {
        if (!this->indices || id < min || id > max) {
            return false;
        }
        return this->indices[id - min] != UINT16_MAX;
    }

    void set(const uint16_t id, const uint16_t value) {
        if (!this->indices || id < min || id > max) {
            this->grow(id, id);
        }
        this->indices[id - min] = value;
    }

    void reset() {
        if (!this->indices) {
            return;
        }
        memset(this->indices, 0xFF, static_cast<size_t>(max - min + 1) * sizeof(uint16_t));
    }

private:
    void grow(const uint16_t newMin, const uint16_t newMax) {
        if (!this->indices) {
            this->min = newMin;
            this->max = newMax;
            const auto count = static_cast<size_t>(newMax - newMin + 1);
            this->indices = new uint16_t[count];
            memset(this->indices, 0xFF, count * sizeof(uint16_t));
            return;
        }

        const uint16_t grownMin = std::min(this->min, newMin);
        const uint16_t grownMax = std::max(this->max, newMax);
        const auto grownCount = static_cast<size_t>(grownMax - grownMin + 1);
        auto* grown = new uint16_t[grownCount];
        memset(grown, 0xFF, grownCount * sizeof(uint16_t));
        memcpy(grown + (this->min - grownMin), this->indices,
               static_cast<size_t>(this->max - this->min + 1) * sizeof(uint16_t));
        delete[] this->indices;
        this->indices = grown;
        this->min = grownMin;
        this->max = grownMax;
    }
};
