#pragma once
#include <functional>
#include <vector>
#include "EcsType.hpp"
#include "EcsVec.hpp"
#include "SystemDesc.hpp"

enum class Phase {
    PreStartup,
    Startup,
    PostStartup,
    PreUpdate,
    Update,
    PostUpdate,
    Physics,
    PreRender,
    Render,
};

class World;

struct SystemRecord {
    QueryId qid;
    std::function<void(const EcsVec<TableId>& tables, World&)> callback;
    std::vector<SystemCondition> conditions;
};

class SystemRegistry {
    using PhaseSystems = std::vector<SystemRecord>;

    PhaseSystems preStartup_;
    PhaseSystems startup_;
    PhaseSystems postStartup_;
    PhaseSystems preUpdate_;
    PhaseSystems update_;
    PhaseSystems postUpdate_;
    PhaseSystems physics_;
    PhaseSystems preRender_;
    PhaseSystems render_;

    [[nodiscard]] PhaseSystems& phaseSystems(Phase phase);
    static void runSystems(PhaseSystems& systems, World&);

public:
    void registerSystem(Phase phase, const SystemRecord& system);
    void runPhase(Phase phase, World&);
};
