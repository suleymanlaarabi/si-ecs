#pragma once
#include <cstdint>
#include <algorithm>
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

template <typename T>
class SparseValues {
    T* values = nullptr;
    size_t capacity = 0;
    size_t count = 0;

public:
    explicit SparseValues() {
        this->grow(4);
    }

    SparseValues(const SparseValues&) = delete;

    SparseValues& operator=(const SparseValues&) = delete;

    SparseValues(SparseValues&& other) noexcept {
        this->values = other.values;
        this->capacity = other.capacity;
        this->count = other.count;
        other.values = nullptr;
        other.capacity = 0;
        other.count = 0;
    }

    SparseValues& operator=(SparseValues&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        delete[] this->values;
        this->values = other.values;
        this->capacity = other.capacity;
        this->count = other.count;
        other.values = nullptr;
        other.capacity = 0;
        other.count = 0;
        return *this;
    }

    ~SparseValues() {
        delete[] this->values;
    }

    [[nodiscard]] const T& at(const uint16_t id) const {
        return this->values[id];
    }

    [[nodiscard]] T& at(const uint16_t id) {
        return this->values[id];
    }

    [[nodiscard]] const T* tryGet(const uint16_t id) const {
        if (id >= this->count) {
            return nullptr;
        }
        return &this->values[id];
    }

    [[nodiscard]] T* tryGet(const uint16_t id) {
        if (id >= this->count) {
            return nullptr;
        }
        return &this->values[id];
    }

    void set(const uint16_t id, const T& value) {
        if (id >= this->capacity) {
            size_t newCapacity = (this->capacity == 0) ? 1 : this->capacity;
            while (id >= newCapacity) {
                newCapacity *= 2;
            }
            this->grow(newCapacity);
        }
        if (id >= this->count) {
            std::fill(this->values + this->count, this->values + id + 1, T{});
            this->count = static_cast<size_t>(id) + 1;
        }
        this->values[id] = value;
    }

    void reset() {
        std::fill(this->values, this->values + this->count, T{});
        this->count = 0;
    }

private:
    void grow(const size_t newCapacity) {
        auto* grown = new T[newCapacity];
        std::fill(grown, grown + newCapacity, T{});
        if (this->count != 0) {
            std::copy_n(this->values, this->count, grown);
        }
        delete[] this->values;
        this->values = grown;
        this->capacity = newCapacity;
    }
};
