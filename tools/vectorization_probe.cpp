#include <cstddef>
#include <iostream>
#include <vector>

int main() {
    constexpr std::size_t size = 4096;

    std::vector<float> a(size, 1.0f);
    std::vector<float> b(size, 2.0f);
    std::vector<float> c(size, 0.0f);

    for (std::size_t i = 0; i < size; ++i) {
        c[i] = a[i] + b[i] * 3.0f;
    }

    float checksum = 0.0f;
    for (std::size_t i = 0; i < size; ++i) {
        checksum += c[i];
    }

    std::cout << checksum << '\n';
    return 0;
}
