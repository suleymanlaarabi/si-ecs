#pragma once
#include <vector>

#include "EcsType.hpp"
#include "TypedTable.hpp"

#define PACK_U16_U32_TO_U64(a, b) \
( (static_cast<std::uint64_t>(a) << 32) | static_cast<std::uint64_t>(b) )

class World;

template <typename E>
using EntityEventCallback = void(*)(World&, Entity, const E&);

using ErasedEntityEventCallback = void(*)(World&, Entity, const void*);


using EntityEventId = uint32_t;

template <typename E>
struct EntityEvent {
    std::vector<EntityEventCallback<E>> listeners;
};

struct EntityEventBase {
    std::vector<ErasedEntityEventCallback> listeners;
};

class EntityEventRegistry {
    std::vector<TypedTable<EntityEventBase>> entityListeners;
    TypedTable<std::vector<EntityEventBase>> globalListeners;

public:
    template <typename E>
    void listen(const Entity entity, void (*callback)(World&, Entity, const E&),
                EntityRegistry& entityRegistry) {
        auto erased = reinterpret_cast<ErasedEntityEventCallback>(callback);

        EntityRecord& record = entityRegistry.getEntityRecord(entity);
        TypedTable<EntityEventBase>* entityListenersTable = nullptr;
        if (record.listenersId != UINT16_MAX) {
            entityListenersTable = &this->entityListeners[record.listenersId];
        } else {
            entityListenersTable = &this->entityListeners.emplace_back();
            record.listenersId = this->entityListeners.size() - 1;
        }
        entityListenersTable->registerElement<E>({
            .listeners = {erased}
        });
    }

    template <typename E>
    EntityEventId listen(void (*callback)(World&, Entity, const E&)) {
        auto erased = reinterpret_cast<ErasedEntityEventCallback>(callback);

        if (!this->globalListeners.isRegistered<E>()) {
            this->globalListeners.registerElement<E>();
        }

        std::vector<EntityEventBase>& listeners = this->globalListeners.getElement<E>();
        listeners.push_back(callback);

        return listeners.size() - 1;
    }

    template <typename E>
    void emit(World& world, const Entity entity, const E& event, EntityRegistry& entityRegistry) {
        if (const EntityRecord& record = entityRegistry.getEntityRecord(entity); record.listenersId != UINT16_MAX) {
            for (TypedTable<EntityEventBase>& entityListenersTable = this->entityListeners[record.listenersId];
                 EntityEventBase& cb : entityListenersTable.getElement<E>()) {
                auto typed = reinterpret_cast<EntityEvent<E>>(cb);
                typed(world, entity, event);
            }
        }

        if (this->globalListeners.isRegistered<E>()) {
            if (std::vector<EntityEventBase>& glisteners = this->globalListeners.getElement<E>(); !glisteners.empty()) {
                for (EntityEventBase& cb : glisteners) {
                    auto typed = reinterpret_cast<EntityEvent<E>>(cb);
                    typed(world, entity, event);
                }
            }
        }
    }
};
