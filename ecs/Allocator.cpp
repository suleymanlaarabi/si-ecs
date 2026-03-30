#include "Allocator.hpp"

[[nodiscard]] std::size_t TempAllocator::availableSize() const {
    return capacity - static_cast<std::size_t>(current - data);
}

[[nodiscard]] void* TempAllocator::allocate(const std::size_t size) {
    if (availableSize() < size) {
        const auto used = static_cast<std::size_t>(this->current - this->data);
        this->capacity = std::max(this->capacity * 2, used + size);
        this->data = static_cast<std::byte*>(std::realloc(this->data, this->capacity));
        this->current = this->data + used;
    }

    std::byte* result = current;
    current += size;
    return result;
}

void TempAllocator::release() {
    this->current = this->data;
}
