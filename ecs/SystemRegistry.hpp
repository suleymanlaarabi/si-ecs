#pragma once
#include <functional>
#include <vector>
#include "EcsType.hpp"
#include "EcsVec.hpp"

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
};


class SystemRegistry {
    std::vector<SystemRecord> preStartup;
    std::vector<SystemRecord> startup;
    std::vector<SystemRecord> postStartup;
    std::vector<SystemRecord> preUpdate;
    std::vector<SystemRecord> update;
    std::vector<SystemRecord> postUpdate;
    std::vector<SystemRecord> physics;
    std::vector<SystemRecord> preRender;
    std::vector<SystemRecord> render;

    static void runSystems(const std::vector<SystemRecord>& systems, World&);

public:
    void registerSystem(Phase phase, const SystemRecord& system);
    void runPhase(Phase phase, World&) const;
};
