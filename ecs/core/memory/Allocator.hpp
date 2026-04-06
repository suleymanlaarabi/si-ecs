#pragma once
#include <cstddef>
#include <cstdlib>

#include "ComponentRegistry.hpp"

struct TempAllocator {
    std::byte* data = static_cast<std::byte*>(malloc(16));
    std::byte* current = data;
    std::size_t capacity = 16;

    TempAllocator() = default;
    TempAllocator(const TempAllocator&) = delete;
    TempAllocator& operator=(const TempAllocator&) = delete;
    TempAllocator(TempAllocator&&) = delete;
    TempAllocator& operator=(TempAllocator&&) = delete;

    ~TempAllocator() {
        std::free(this->data);
    }

    [[nodiscard]] std::size_t availableSize() const;

    [[nodiscard]] void* allocate(std::size_t size);

    void release();
};
