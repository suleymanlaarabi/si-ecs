#include "SystemRegistry.hpp"

#include "Commands.hpp"
#include "World.hpp"

SystemRegistry::PhaseSystems& SystemRegistry::phaseSystems(const Phase phase) {
    switch (phase) {
    case Phase::PreStartup:
        return this->preStartup_;
    case Phase::Startup:
        return this->startup_;
    case Phase::PostStartup:
        return this->postStartup_;
    case Phase::PreUpdate:
        return this->preUpdate_;
    case Phase::Update:
        return this->update_;
    case Phase::PostUpdate:
        return this->postUpdate_;
    case Phase::Physics:
        return this->physics_;
    case Phase::PreRender:
        return this->preRender_;
    case Phase::Render:
        return this->render_;
    }

    __builtin_unreachable();
}

void SystemRegistry::registerSystem(const Phase phase, const SystemRecord &system) {
    this->phaseSystems(phase).emplace_back(system);
}

void SystemRegistry::runSystems(PhaseSystems &systems, World &world) {
    for (auto &[qid, callback, conditions]: systems) {
        bool shouldRun = true;
        for (auto &[ctx, check]: conditions) {
            if (!check(ctx, world)) {
                shouldRun = false;
                break;
            }
        }

        if (!shouldRun) {
            continue;
        }

        callback(world.getQueries()[qid].tables, world);
    }
}

void SystemRegistry::runPhase(const Phase phase, World &world) {
    runSystems(this->phaseSystems(phase), world);
    world.getSingleton<Commands>().flush();
}
