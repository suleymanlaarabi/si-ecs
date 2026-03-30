#include "SystemRegistry.hpp"

#include "Commands.hpp"
#include "World.hpp"

void SystemRegistry::registerSystem(const Phase phase, const SystemRecord& system) {
    if (phase == Phase::PreStartup) {
        preStartup.emplace_back(system);
    } else if (phase == Phase::Startup) {
        startup.emplace_back(system);
    } else if (phase == Phase::PostStartup) {
        postStartup.emplace_back(system);
    } else if (phase == Phase::PreUpdate) {
        preUpdate.emplace_back(system);
    } else if (phase == Phase::Update) {
        update.emplace_back(system);
    } else if (phase == Phase::PostUpdate) {
        postUpdate.emplace_back(system);
    } else if (phase == Phase::Physics) {
        physics.emplace_back(system);
    } else if (phase == Phase::PreRender) {
        preRender.emplace_back(system);
    } else if (phase == Phase::Render) {
        render.emplace_back(system);
    }
}

void SystemRegistry::runSystems(const std::vector<SystemRecord>& systems, World& world) {
    for (const auto& [qid, callback] : systems) {
        callback(world.getQueries()[qid].tables, world);
    }
}

void SystemRegistry::runPhase(const Phase phase, World& world) const {
    if (phase == Phase::PreStartup) {
        runSystems(this->preStartup, world);
    } else if (phase == Phase::Startup) {
        runSystems(this->startup, world);
    } else if (phase == Phase::PostStartup) {
        runSystems(this->postStartup, world);
    } else if (phase == Phase::PreUpdate) {
        runSystems(this->preUpdate, world);
    } else if (phase == Phase::Update) {
        runSystems(this->update, world);
    } else if (phase == Phase::PostUpdate) {
        runSystems(this->postUpdate, world);
    } else if (phase == Phase::Physics) {
        runSystems(this->physics, world);
    } else if (phase == Phase::PreRender) {
        runSystems(this->preRender, world);
    } else if (phase == Phase::Render) {
        runSystems(this->render, world);
    }
    world.getSingleton<Commands>().flush();
}
