#pragma once
#include <cstdint>
#include <cstring>

class SparseIndices {
    uint16_t *indices = nullptr;
    size_t capacity = 0;
    size_t count = 0;

public:
    explicit SparseIndices() {
        this->grow(4);
    }

    SparseIndices(const SparseIndices &) = delete;

    SparseIndices &operator=(const SparseIndices &) = delete;

    SparseIndices(SparseIndices &&other) noexcept {
        this->indices = other.indices;
        this->capacity = other.capacity;
        this->count = other.count;
        other.indices = nullptr;
        other.capacity = 0;
        other.count = 0;
    }

    SparseIndices &operator=(SparseIndices &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        delete[] this->indices;
        this->indices = other.indices;
        this->capacity = other.capacity;
        this->count = other.count;
        other.indices = nullptr;
        other.capacity = 0;
        other.count = 0;
        return *this;
    }

    ~SparseIndices() {
        delete[] this->indices;
    }

    [[nodiscard]] uint16_t at(const uint16_t id) const {
        return this->indices[id];
    }

    [[nodiscard]] uint16_t atOrInvalid(const uint16_t id) const {
        if (id >= this->count) {
            return UINT16_MAX;
        }
        return this->indices[id];
    }

    [[nodiscard]] bool has(const uint16_t id) const {
        if (id >= this->count) {
            return false;
        }
        return this->indices[id] != UINT16_MAX;
    }

    void set(const uint16_t id, const uint16_t value) {
        if (id >= this->capacity) {
            size_t newCapacity = (this->capacity == 0) ? 1 : this->capacity;
            while (id >= newCapacity) {
                newCapacity *= 2;
            }
            this->grow(newCapacity);
        }
        if (id >= this->count) {
            memset(this->indices + this->count, 0xFF, static_cast<size_t>(id - this->count + 1) * sizeof(uint16_t));
            this->count = static_cast<size_t>(id) + 1;
        }
        this->indices[id] = value;
    }

    void reset() {
        if (this->count == 0) {
            return;
        }
        memset(this->indices, 0xFF, static_cast<size_t>(this->count) * sizeof(uint16_t));
        this->count = 0;
    }

private:
    void grow(const size_t newCapacity) {
        auto *grown = new uint16_t[newCapacity];
        memset(grown, 0xFF, static_cast<size_t>(newCapacity) * sizeof(uint16_t));
        if (this->count != 0) {
            memcpy(grown, this->indices, static_cast<size_t>(this->count) * sizeof(uint16_t));
        }
        delete[] this->indices;
        this->indices = grown;
        this->capacity = newCapacity;
    }
};
