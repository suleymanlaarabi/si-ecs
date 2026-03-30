#pragma once
#include <utility>
#include "ComponentRegistry.hpp"
#include "EntityManager.hpp"
#include "EntityEventRegistry.hpp"
#include "EventRegistry.hpp"
#include "Iter.hpp"
#include "PluginRegistry.hpp"
#include "QueryRegistry.hpp"
#include "QueryTables.hpp"
#include "Relations.hpp"
#include "SingletonRegistry.hpp"
#include "SystemRegistry.hpp"
#include "TableRegistry.hpp"

class World;
struct SystemDesc;

class World : EntityManager, public SingletonRegistry, public PluginRegistry, SystemRegistry,
              public EventRegistry, public EntityEventRegistry, public QueryRegistry {
public:
    using EntityManager::createEntity;
    using EntityManager::isAlive;
    using EntityManager::registerComponent;
    using EntityManager::addComponent;
    using EntityManager::addBatchComponents;
    using EntityManager::removeComponent;
    using EntityManager::setComponent;
    using EntityManager::getTable;
    using EventRegistry::listen;
    using TableRegistry::getTables;
    using EventRegistry::unlisten;

    World();
    void destroyEntity(Entity entity);

    template <typename T>
    void add(const Entity entity) {
        this->addComponent(entity, ComponentRegistry::id<T>());
    }

    void system(Phase phase, const SystemDesc& desc);
    QueryId cacheQuery(const Query& desc);
    [[nodiscard]] RuntimeQuery query();
    [[nodiscard]] RuntimeQuery query(const Query& desc);

    template <typename Func>
    void eachMatchingTable(const Query& desc, Func&& func) {
        forEachMatchingTable(desc, *this, *this, std::forward<Func>(func));
    }

    template <typename T>
    void registerPlugin() {
        PluginRegistry::registerPlugin<T>(*this);
    }

    template <typename T>
    void set(const Entity entity, const T& value) {
        this->setComponent(entity, ComponentRegistry::id<T>(), &value);
    }

    template <typename T>
    void set(const Entity entity, T&& temp) {
        T value = std::forward<T>(temp);
        this->setComponent(entity, ComponentRegistry::id<T>(), &value);
    }

    template <typename T>
    void remove(const Entity entity) {
        this->removeComponent(entity, ComponentRegistry::id<T>());
    }

    template <typename E>
    void emit(const E& event) {
        EventRegistry::emit(*this, event);
    }

    template <typename E>
    EntityEventId listen(void (*callback)(World&, Entity, const E&)) {
        return EntityEventRegistry::listen<E>(callback);
    }

    template <typename E>
    void listen(const Entity entity, void (*callback)(World&, Entity, const E&)) {
        return EntityEventRegistry::listen<E>(entity, callback);
    }

    template <typename E>
    void emit(const Entity entity, const E& event) {
        EntityEventRegistry::emit(*this, entity, event);
    }

    template <typename E>
    void unlisten(const EntityEventId id) {
        // TODO: unlisten
        //EntityEventRegistry::unlisten<E>(id);
    }

    template <typename T>
    bool has(const Entity entity) {
        return this->hasComponent(entity, ComponentRegistry::id<T>());
    }

    template <typename T>
    T& get(const Entity entity) {
        return *static_cast<T*>(this->getComponent(entity, ComponentRegistry::id<T>()));
    }

    template <typename T>
    void registerRelation() {
        this->registerComponent<RelationTarget<T>>();
        this->registerComponent<RelationSource<T>>();
    }

    template <typename T>
    void relate(const Entity entity, const Entity target) {
        this->set(entity, RelationTarget<T>(target));
    }

    template <typename T>
    void unrelate(const Entity entity, const Entity target) {
        if (!this->hasTarget<T>(entity, target)) {
            return;
        }

        this->remove<RelationTarget<T>>(entity);
    }

    template <typename T>
    bool hasTarget(const Entity entity, const Entity target) {
        return this->has<RelationTarget<T>>(entity) && this->get<RelationTarget<T>>(entity).target == target;
    }

    template <typename T>
    bool hasSource(const Entity entity, const Entity target) {
        return this->has<RelationSource<T>>(entity) && this->get<RelationSource<T>>(entity).entities.has(target);
    }

    template <typename T>
    Entity getTarget(const Entity entity, const Entity target) {
        return this->get<RelationTarget<T>>(entity).target;
    }

    void progress();
    [[noreturn]] void run();
    void start();

    void shrink();

private:
    void removeTableIfEmpty(TableId tid);
};


template <typename T>
void RelationTarget<T>::onSet(Entity entity, const RelationTarget& target, World& world) {
    if (world.has<RelationTarget>(entity)) {
        const RelationTarget& currentTarget = world.get<RelationTarget>(entity);
        auto& source = world.get<RelationSource<T>>(currentTarget.target);
        source.entities.remove(entity);

        if (source.entities.empty()) {
            world.remove<RelationSource<T>>(currentTarget.target);
        }
    }

    if (world.has<RelationSource<T>>(target.target)) {
        world.get<RelationSource<T>>(target.target).entities.push_back(entity);
    } else {
        world.set<RelationSource<T>>(target.target, std::move(RelationSource<T>{EcsVec(entity)}));
    }
}

template <typename T>
void RelationTarget<T>::onRemove(const Entity entity, World& world) {
    const Entity target = world.get<RelationTarget>(entity).target;

    auto& source = world.get<RelationSource<T>>(target);
    source.entities.remove(entity);

    if (source.entities.empty()) {
        world.remove<RelationSource<T>>(target);
    }
}
