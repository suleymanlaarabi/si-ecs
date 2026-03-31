#pragma once
#include <string>
#include "ComponentType.hpp"
#include "EcsType.hpp"
#include "TypedTable.hpp"
#include "EcsVec.hpp"

class World;

struct ComponentRecord {
    EcsVec<ComponentId> required;
    EcsVec<TableId> tables;
    std::string name;
    void (*onAdd)(Entity entity, void*) = nullptr;
    void (*onRemove)(Entity entity, void*) = nullptr;
    void (*onSet)(Entity entity, const void*, void*) = nullptr;
    uint16_t size = 0;
};

template <typename Component>
concept HasOnAdd = requires(Entity entity, World& world) {
    { Component::onAdd(entity, world) };
};

template <typename Component>
concept HasOnRemove = requires(Entity entity, World& world) {
    { Component::onRemove(entity, world) };
};

template <typename Component>
concept HasOnSet = requires(Entity entity, Component& newValue, World& world) {
    { Component::onSet(entity, newValue, world) };
};

template <typename Component>
concept HasRequired = requires {
    { Component::required() } -> std::same_as<EcsVec<ComponentId>>;
};

class ComponentRegistry : ComponentType, TypedTable<ComponentRecord, ComponentType> {
    EcsVec<ComponentId> resolveRequiredComponentIds(const EcsVec<ComponentId>& directRequired);

public:
    using ComponentType::id;

#ifndef NDEBUG
    [[nodiscard]] bool isComponentRegistered(const uint16_t id) const {
        return this->isRegistered(id);
    }
#endif

    [[nodiscard]] const ComponentRecord& getComponentRecord(ComponentId cid) const;
    [[nodiscard]] ComponentRecord& getComponentRecord(ComponentId cid);

    template <typename Component>
    void registerComponent() {
        if (this->isRegistered<Component>()) {
            return;
        }
        ComponentRecord record;


        if constexpr (HasOnAdd<Component>) {
            record.onAdd = reinterpret_cast<void(*)(Entity entity, void*)>(Component::onAdd);
        }
        if constexpr (HasOnRemove<Component>) {
            record.onRemove = reinterpret_cast<void(*)(Entity entity, void*)>(Component::onRemove);
        }
        if constexpr (HasOnSet<Component>) {
            record.onSet = reinterpret_cast<void(*)(Entity entity, const void*, void*)>(Component::onSet);
        }

        record.name = type_name_string<Component>();

        record.size = ecs_sizeof<Component>();

        if constexpr (HasRequired<Component>) {
            record.required = this->resolveRequiredComponentIds(Component::required());
        }

        this->registerElement<Component>(record);
    }
};

template <typename... Components>
struct Required {
    static EcsVec<ComponentId> required() {
        EcsVec<ComponentId> result;
        (result.push_back(ComponentRegistry::id<Components>()), ...);

        return result;
    }

    Required() = default;

    bool operator==(const Required&) const {
        return true;
    }
};
