#pragma once
#include <tuple>
#include <type_traits>
#include <utility>

#include "ComponentRegistry.hpp"
#include "EcsType.hpp"
#include "EntityManager.hpp"
#include "EntityEventRegistry.hpp"
#include "EventRegistry.hpp"
#include "Iter.hpp"
#include "PluginRegistry.hpp"
#include "QueryRegistry.hpp"
#include "QueryTables.hpp"
#include "Relations.hpp"
#include "SingletonRegistry.hpp"
#include "SystemDesc.hpp"
#include "SystemRegistry.hpp"
#include "System.hpp"
#include "typing.hpp"

class World;
struct SystemDesc;

class World {
    EntityManager entityManager_;
    SingletonRegistry singletonRegistry_;
    PluginRegistry pluginRegistry_;
    SystemRegistry systemRegistry_;
    EventRegistry eventRegistry_;
    EntityEventRegistry entityEventRegistry_;
    QueryRegistry queryRegistry_;
    float deltaTime = 0;

public:
    World();

    ~World();

    [[nodiscard]] Entity createEntity() {
        return this->entityManager_.createEntity();
    }

    void destroyEntity(const Entity entity) {
        this->entityManager_.destroyEntity(entity);
    }

    [[nodiscard]] bool isAlive(const Entity entity) const {
        return this->entityManager_.isAlive(entity);
    }

    template <typename T>
    void registerComponent() {
        this->entityManager_.registerComponent<T>();
    }

    void addComponent(const Entity entity, const ComponentId cid) {
        this->entityManager_.addComponent(entity, cid);
    }

    void removeComponent(const Entity entity, const ComponentId cid) {
        this->entityManager_.removeComponent(entity, cid);
    }

    void setComponent(const Entity entity, const ComponentId cid, const void* component) {
        this->entityManager_.setComponent(entity, cid, component);
    }

    [[nodiscard]] bool hasComponent(const Entity entity, const ComponentId cid) {
        return this->entityManager_.hasComponent(entity, cid);
    }

    [[nodiscard]] void* getComponent(const Entity entity, const ComponentId cid) {
        return this->entityManager_.getComponent(entity, cid);
    }

    std::pair<Table&, EntityRow> getTable(const Entity entity) {
        return this->entityManager_.getTable(entity);
    }

    std::vector<Table*>& getTables() {
        return this->entityManager_.getTables();
    }

    template <typename T>
    void add(const Entity entity) {
        this->addComponent(entity, ComponentRegistry::id<T>());
    }

    void system(Phase phase, const SystemDesc& desc);

    template<auto func, typename ...Terms>
    void onPreUpdate() {
        this->system(Phase::PreUpdate, each<func, Terms...>());
    }
    template<auto func, typename ...Terms>
    void onUpdate() {
        this->system(Phase::Update, each<func, Terms...>());
    }
    template<auto func, typename ...Terms>
    void onPostUpdate() {
        this->system(Phase::PostUpdate, each<func, Terms...>());
    }

    QueryId cacheQuery(const Query& desc);

    [[nodiscard]] RuntimeQuery query();

    [[nodiscard]] RuntimeQuery query(const Query& desc);

    template <typename Func>
    void eachMatchingTable(const Query& desc, Func&& func) {
        forEachMatchingTable(desc, this->entityManager_, this->entityManager_, std::forward<Func>(func));
    }

    template <typename T>
    void registerPlugin() {
        this->pluginRegistry_.registerPlugin<T>(*this);
    }

    template <typename T>
    void set(const Entity entity, const T& value) {
        this->setComponent(entity, ComponentRegistry::id<T>(), &value);
    }

    template <typename T>
    void set(const Entity entity, T&& temp) {
        using Component = std::remove_cvref_t<T>;
        if constexpr (ecs_sizeof<Component>() == 0) {
            this->add<Component>(entity);
            return;
        }

        Component value = std::forward<T>(temp);
        this->setComponent(entity, ComponentRegistry::id<Component>(), &value);
    }

    template <typename T>
    void remove(const Entity entity) {
        this->removeComponent(entity, ComponentRegistry::id<T>());
    }

    template <typename E>
    void emit(const E& event) {
        this->eventRegistry_.emit(*this, event);
    }

    template <typename E>
    EntityEventId listen(void (*callback)(World&, Entity, const E&)) {
        return this->entityEventRegistry_.listen<E>(callback);
    }

    template <typename E>
    EventId listen(void (*callback)(World&, const E&)) {
        return this->eventRegistry_.listen<E>(callback);
    }

    template <typename E>
    void listen(const Entity entity, void (*callback)(World&, Entity, const E&)) {
        this->entityEventRegistry_.listen<E>(entity, callback, this->entityManager_);
    }

    template <typename E>
    void emit(const Entity entity, const E& event) {
        this->entityEventRegistry_.emit(*this, entity, event, this->entityManager_);
    }

    template <typename E>
    void unlisten(EventId id) {
        this->eventRegistry_.unlisten<E>(id);
    }

    template <typename E>
    void unlisten(const Entity entity, EntityEventId id) {
        // TODO
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

    template <typename T>
    T& getSingleton() {
        return this->singletonRegistry_.getSingleton<T>();
    }

    template <typename T>
    bool hasSingleton() {
        return this->singletonRegistry_.hasSingleton<T>();
    }

    template <typename T>
    void initSingleton() {
        this->singletonRegistry_.initSingleton<T>();
    }

    template <typename T>
    void initSingleton(T* value) {
        this->singletonRegistry_.initSingleton<T>(value);
    }

    QueryId registerQuery(const Query& query) {
        return this->queryRegistry_.registerQuery(query, this->entityManager_, this->entityManager_);
    }

    std::vector<QueryCache>& getQueries() {
        return this->queryRegistry_.getQueries();
    }

    void progress();

    [[noreturn]] void run();

    void start();

    [[nodiscard]] float getDeltaTime() const {
        return this->deltaTime;
    }
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
void RelationSource<T>::onRemove(const Entity entity, World& world) {
    world.get<RelationSource<T>>(entity).entities.free();
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

struct Timer {
    float interval;
    float elapsed = 0;

    explicit Timer(const float interval) : interval(interval) {}

    bool tick(const float dt) {
        elapsed += dt;

        if (elapsed >= interval) {
            elapsed -= interval;
            return true;
        }
        return false;
    }
};
