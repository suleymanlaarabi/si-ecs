#pragma once
#include <chrono>
#include <iostream>
#include <string>

struct BenchTimer {
    std::string name;
    std::chrono::high_resolution_clock::time_point start;

    explicit BenchTimer(std::string name) : name(std::move(name)), start(std::chrono::high_resolution_clock::now()) {}

    ~BenchTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << name << ": " << duration << "ms" << std::endl;
    }
};
