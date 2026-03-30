#pragma once
#include <cstdint>
#include <cstring>

class SparseIndices {
    uint16_t* indices = nullptr;
    uint16_t min = 0;
    uint16_t max = 0;

public:
    SparseIndices(const uint16_t min, const uint16_t max) {
        this->min = min;
        this->max = max;

        const auto count = max - min + 1;
        this->indices = new uint16_t[count];
        memset(this->indices, 0xFF, count * sizeof(uint16_t));
    };

    [[nodiscard]] uint16_t at(const uint16_t id) const {
        return this->indices[id - min];
    }

    [[nodiscard]] bool has(const uint16_t id) const {
        if (id < min || id > max) {
            return false;
        }
        return this->indices[id - min] != UINT16_MAX;
    }

    void set(const uint16_t id, const uint16_t value) const {
        this->indices[id - min] = value;
    }
};
