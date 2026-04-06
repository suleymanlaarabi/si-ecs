#pragma once
#include <cstdlib>
#include <cstring>

template<typename T, typename Size = uint32_t>
struct EcsVec {
    T *data = nullptr;
    Size size = 0;
    Size capacity = 0;

    EcsVec() = default;

    EcsVec(const EcsVec &other) {
        if (other.size == 0) {
            return;
        }

        this->data = static_cast<T *>(std::malloc(other.size * sizeof(T)));
        std::memcpy(this->data, other.data, other.size * sizeof(T));
        this->size = other.size;
        this->capacity = other.size;
    }

    EcsVec(EcsVec &&other) noexcept
        : data(other.data), size(other.size), capacity(other.capacity) {
        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;
    }

    EcsVec &operator=(const EcsVec &other) {
        if (this == &other) {
            return *this;
        }

        this->free();
        if (other.size == 0) {
            return *this;
        }

        this->data = static_cast<T *>(std::malloc(other.size * sizeof(T)));
        std::memcpy(this->data, other.data, other.size * sizeof(T));
        this->size = other.size;
        this->capacity = other.size;
        return *this;
    }

    EcsVec &operator=(EcsVec &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        this->free();
        this->data = other.data;
        this->size = other.size;
        this->capacity = other.capacity;
        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;
        return *this;
    }

    explicit EcsVec(T value) {
        this->push_back(value);
    }

    EcsVec(Size capacity, T value) : capacity(capacity) {
        this->data = static_cast<T *>(std::malloc(capacity * sizeof(T)));
        this->capacity = capacity;
        for (Size i = 0; i < capacity; ++i) {
            std::memcpy(this->data + i, &value, sizeof(T));
        }
    }

    void push_back(const T &value) {
        if (this->size >= this->capacity) {
            this->capacity = this->capacity == 0 ? 1 : this->capacity * 2;
            this->data = static_cast<T *>(std::realloc(this->data, this->capacity * sizeof(T)));
        }
        std::memcpy(this->data + this->size, &value, sizeof(T));
        this->size += 1;
    }


    void erase(Size index) {
        uint32_t last = this->size - 1;
        if (index != last)
            std::memcpy(this->data + index, this->data + last, sizeof(T));
        this->size -= 1;
    }

    void remove(const T &value) {
        for (uint32_t i = 0; i < this->size; i++) {
            if (this->data[i] == value) {
                this->erase(i);
                return;
            }
        }
    }

    void replace(const T &value, const T &newValue) {
        for (uint32_t i = 0; i < this->size; i++) {
            if (this->data[i] == value) {
                this->data[i] = newValue;
                return;
            }
        }
    }

    bool has(const T &value) {
        for (uint32_t i = 0; i < this->size; i++) {
            if (this->data[i] == value) {
                return true;
            }
        }
        return false;
    }

    void resize(Size new_size) {
        if (new_size > this->capacity) {
            this->capacity = new_size;
            this->data = static_cast<T *>(std::realloc(this->data, this->capacity * sizeof(T)));
        }
        this->size = new_size;
    }

    void resize(Size new_size, const T &value) {
        if (new_size > this->capacity) {
            this->capacity = new_size;
            this->data = static_cast<T *>(std::realloc(this->data, this->capacity * sizeof(T)));
        }
        std::fill(this->data + this->size, this->data + new_size, value);
        this->size = new_size;
    }

    void pop_back() {
        if (this->size > 0) {
            this->size -= 1;
        }
    }

    void free() {
        std::free(this->data);
        this->data = nullptr;
        this->size = 0;
        this->capacity = 0;
    }

    T *begin() {
        return this->data;
    }

    T *end() {
        return this->data + this->size;
    }

    const T *begin() const {
        return this->data;
    }

    const T *end() const {
        return this->data + this->size;
    }

    T &operator[](Size i) {
        return this->data[i];
    }

    const T &operator[](Size i) const {
        return this->data[i];
    }

    [[nodiscard]] bool empty() const {
        return this->size == 0;
    }

    EcsVec clone() const {
        return EcsVec(*this);
    }
};
