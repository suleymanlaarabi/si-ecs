#pragma once
#include "TypedTable.hpp"

class SingletonRegistry : TypedTable<void*> {
public:
    template <typename T>
    T& getSingleton() {
        return *static_cast<T*>(this->getElement<T>());
    }

    template <typename T>
    bool hasSingleton() {
        return this->isRegistered<T>() && this->getElement<T>() != nullptr;
    }

    template <typename T>
    void initSingleton() {
        if (!this->isRegistered<T>()) {
            this->registerElement<T>(nullptr);
        }
        this->getElement<T>() = new T();
    }

    template <typename T>
    void initSingleton(T* value) {
        if (!this->isRegistered<T>()) {
            this->registerElement<T>(nullptr);
        }
        this->getElement<T>() = value;
    }
};
