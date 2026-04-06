#pragma once
#include "TypedTable.hpp"

struct SingletonEntry {
    void* value = nullptr;
    void (*destroy)(void*) = nullptr;
};

class SingletonRegistry : TypedTable<SingletonEntry> {
public:
    ~SingletonRegistry() {
        this->forEachRegistered([](const uint16_t, SingletonEntry& entry) {
            if (entry.destroy != nullptr) {
                entry.destroy(entry.value);
            }
            entry.value = nullptr;
            entry.destroy = nullptr;
        });
    }

    template <typename T>
    T& getSingleton() {
        return *static_cast<T*>(this->getElement<T>().value);
    }

    template <typename T>
    bool hasSingleton() {
        return this->isRegistered<T>() && this->getElement<T>().value != nullptr;
    }

    template <typename T>
    void initSingleton() {
        auto* value = new T();
        this->initSingleton<T>(value);
    }

    template <typename T>
    void initSingleton(T* value) {
        if (!this->isRegistered<T>()) {
            this->registerElement<T>({
                .value = value,
                .destroy = nullptr
            });
            return;
        }

        SingletonEntry& entry = this->getElement<T>();
        if (entry.destroy != nullptr) {
            entry.destroy(entry.value);
        }
        entry.value = value;
        entry.destroy = nullptr;
    }
};
