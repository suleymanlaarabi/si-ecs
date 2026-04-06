#pragma once
#include "TypedTable.hpp"
#include <vector>

class World;

template <typename E>
using EventCallback = void(*)(World&, const E&);

using ErasedEventCallback = void(*)(World&, const void*);


using EventId = uint16_t;

template <typename E>
struct Event {
    std::vector<EventCallback<E>> listeners;
};

struct EventBase {
    std::vector<ErasedEventCallback> listeners;
};

class EventRegistry : TypedTable<EventBase> {
public:
    template <typename E>
    EventId listen(void (*callback)(World&, const E&)) {
        auto erased = reinterpret_cast<ErasedEventCallback>(callback);

        if (!this->isRegistered<E>()) {
            this->registerElement<E>({
                .listeners = {erased}
            });
            return 0;
        }

        auto& base = this->getElement<E>();
        base.listeners.push_back(erased);
        return static_cast<EventId>(base.listeners.size() - 1);
    }

    template <typename E>
    void emit(World& world, const E& event) {
        if (!this->isRegistered<E>()) {
            return;
        }

        for (auto& base = this->getElement<E>(); auto cb : base.listeners) {
            auto typed = reinterpret_cast<EventCallback<E>>(cb);
            typed(world, event);
        }
    }

    template <typename E>
    void unlisten(EventId id) {
        auto& listeners = this->getElement<E>().listeners;
        listeners.erase(listeners.begin() + id);
    }
};
