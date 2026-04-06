#include "World.hpp"
#include "Addons.hpp"
#include "Commands.hpp"
#include "SystemDesc.hpp"

std::vector<Table>& worldTables(World& world) { return world.getTables(); }

std::pair<Table&, EntityRow> worldGetTable(World& world, const Entity entity) {
    return world.getTable(entity);
}

World::World() {
    this->registerRelation<ChildOf>();
    this->initSingleton<Commands>(new Commands(*this));
    this->initSingleton<DeltaTime>();
}

World::~World() {
    for (Table& table : this->entityManager_.getTables()) {
        if (!table.hasComponent(ComponentRegistry::id<RelationSource<ChildOf>>())) {
            continue;
        }

        auto* sources = static_cast<RelationSource<ChildOf>*>(
            table.getColumn(ComponentRegistry::id<RelationSource<ChildOf>>()));
        for (EntityRow row = 0; row < table.size(); ++row) {
            sources[row].entities.free();
        }
    }
}

void World::system(const Phase phase, const SystemDesc& desc) {
    this->systemRegistry_.registerSystem(
        phase, {
            .qid = this->queryRegistry_.registerQuery(
                desc.query, this->entityManager_, this->entityManager_),
            .callback = desc.callback,
            .conditions = desc.conditions,
        });
}

QueryId World::cacheQuery(const Query& desc) {
    return this->queryRegistry_.registerQuery(desc, this->entityManager_,
                                              this->entityManager_);
}

RuntimeQuery World::query() { return *this; }

RuntimeQuery World::query(const Query& desc) { return {*this, desc}; }

void World::progress() {
    this->systemRegistry_.runPhase(Phase::PreUpdate, *this);
    this->systemRegistry_.runPhase(Phase::Update, *this);
    this->systemRegistry_.runPhase(Phase::PostUpdate, *this);
    this->systemRegistry_.runPhase(Phase::Physics, *this);
    this->systemRegistry_.runPhase(Phase::PreRender, *this);
    this->systemRegistry_.runPhase(Phase::Render, *this);
}

[[noreturn]] void World::run() {
    this->start();
    while (true) {
        this->progress();
    }
}

void World::start() {
    this->systemRegistry_.runPhase(Phase::PreStartup, *this);
    this->systemRegistry_.runPhase(Phase::Startup, *this);
    this->systemRegistry_.runPhase(Phase::PostStartup, *this);
}

void World::removeTableIfEmpty(const TableId tid) {
    TableMap& tableMap = this->entityManager_.getTableMap();
    if (tableMap.tables[tid].count != 0) {
        return;
    }

    const EntityType removedType = tableMap.tables[tid].getType().clone();
    const TableId swapped = tableMap.remove(tid);

    for (uint16_t i = 0; i < removedType.count; ++i) {
        this->entityManager_.getComponentRecord(removedType.cids[i])
            .tables.remove(tid);
    }

    if (swapped != InvalidTableId) {
        const Table& movedTable = tableMap.tables[tid];
        const EntityType& movedType = movedTable.getType();

        for (uint16_t i = 0; i < movedType.count; ++i) {
            this->entityManager_.getComponentRecord(movedType.cids[i])
                .tables.replace(swapped, tid);
        }

        for (EntityRow row = 0; row < movedTable.size(); ++row) {
            this->entityManager_.getEntityRecord(movedTable.getEntities()[row]).tid =
                tid;
        }
    }

    for (auto& [query, tables] : this->queryRegistry_.getQueries()) {
        tables.remove(tid);
        tables.replace(swapped, tid);
    }

    removedType.release();
}

void World::shrink() {
    std::vector<Table>& tables = this->entityManager_.getTableMap().tables;
    bool removedTable = false;
    for (uint i = 0; i < tables.size(); i++) {
        const size_t before = tables.size();
        this->removeTableIfEmpty(i);
        removedTable |= tables.size() != before;
    }

    if (!removedTable) {
        return;
    }

    for (Table& table : tables) {
        table.resetEdges();
    }
}
