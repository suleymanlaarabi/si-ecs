#pragma once
#include <cstddef>

#include "ComponentRegistry.hpp"

struct TempAllocator {
    std::byte* data = static_cast<std::byte*>(malloc(16));
    std::byte* current = data;
    std::size_t capacity = 16;

    [[nodiscard]] std::size_t availableSize() const;

    [[nodiscard]] void* allocate(std::size_t size);

    void release();
};
