#pragma once
#include "ComponentType.hpp"

template <typename T, typename IdTag = T>
class TypedTable {
    std::vector<T> dense;
    std::vector<uint64_t> registered_bits;

public:
    template <typename E>
    static uint16_t id() {
        return TypeIdRegistry<IdTag>::template id<E>();
    }

    template <typename E>
    [[nodiscard]] bool isRegistered() const {
        const uint16_t id = this->id<E>();
        return this->isRegistered(id);
    }

    [[nodiscard]] bool isRegistered(const uint16_t id) const {
        const size_t word = id / 64;
        if (word >= this->registered_bits.size()) {
            return false;
        }

        return (this->registered_bits[word] & (uint64_t{1} << (id % 64))) != 0;
    }

    template <typename E>
    void registerElement(T value) {
        const uint16_t id = this->id<E>();
        if (this->dense.size() <= id) {
            this->dense.resize(id + 1);
        }

        const size_t word = id / 64;
        if (this->registered_bits.size() <= word) {
            this->registered_bits.resize(word + 1, 0);
        }

        this->dense[id] = std::move(value);
        this->registered_bits[word] |= uint64_t{1} << (id % 64);
    }


    template <typename E>
    T& getElement() {
        return this->dense[this->id<E>()];
    }

    T& getElementById(uint16_t id) {
        return this->dense[id];
    }

    const T& getElementById(uint16_t id) const {
        return this->dense[id];
    }
};
