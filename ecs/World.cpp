#include "World.hpp"
#include "Addons.hpp"
#include "Commands.hpp"
#include "SystemDesc.hpp"

std::vector<Table>& worldTables(World& world) {
    return world.getTables();
}

std::pair<Table&, EntityRow> worldGetTable(World& world, const Entity entity) {
    return world.getTable(entity);
}

World::World() {
    this->registerRelation<ChildOf>();
    this->initSingleton<Commands>(new Commands(
        *this
    ));
    this->initSingleton<DeltaTime>();
}

void World::destroyEntity(const Entity entity) {
    EntityManager::destroyEntity(entity);
}

void World::system(const Phase phase, const SystemDesc& desc) {
    this->registerSystem(phase, {
                             .qid = this->registerQuery(desc.query, *this, *this),
                             .callback = desc.callback
                         });
}

QueryId World::cacheQuery(const Query& desc) {
    return this->registerQuery(desc, *this, *this);
}

RuntimeQuery World::query() {
    return RuntimeQuery(*this);
}

RuntimeQuery World::query(const Query& desc) {
    return RuntimeQuery(*this, desc);
}

void World::progress() {
    this->runPhase(Phase::PreUpdate, *this);
    this->runPhase(Phase::Update, *this);
    this->runPhase(Phase::PostUpdate, *this);
    this->runPhase(Phase::Physics, *this);
    this->runPhase(Phase::PreRender, *this);
    this->runPhase(Phase::Render, *this);
}

[[noreturn]] void World::run() {
    this->start();
    while (true) {
        this->progress();
    }
}

void World::start() {
    this->runPhase(Phase::PreStartup, *this);
    this->runPhase(Phase::Startup, *this);
    this->runPhase(Phase::PostStartup, *this);
}

void World::removeTableIfEmpty(const TableId tid) {
    if (this->tables_map.tables[tid].count != 0) {
        return;
    }
    const EntityType removedType = this->tables_map.tables[tid].getType().clone();
    const TableId swapped = this->tables_map.remove(tid);

    for (uint16_t i = 0; i < removedType.count; ++i) {
        this->getComponentRecord(removedType.cids[i]).tables.remove(tid);
    }

    if (swapped != InvalidTableId) {
        Table& movedTable = this->tables_map.tables[tid];
        const EntityType& movedType = movedTable.getType();

        for (uint16_t i = 0; i < movedType.count; ++i) {
            this->getComponentRecord(movedType.cids[i]).tables.replace(swapped, tid);
        }

        for (EntityRow row = 0; row < movedTable.size(); ++row) {
            this->getEntityRecord(movedTable.getEntities()[row]).tid = tid;
        }
    }

    for (auto& [query, tables] : this->getQueries()) {
        tables.remove(tid);
        tables.replace(swapped, tid);
    }

    removedType.release();
}

void World::shrink() {
    std::vector<Table>& tables = this->tables_map.tables;
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
