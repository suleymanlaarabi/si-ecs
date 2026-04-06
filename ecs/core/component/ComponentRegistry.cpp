#include "ComponentRegistry.hpp"
#include <algorithm>
#include "EcsAssert.hpp"

[[nodiscard]] const ComponentRecord& ComponentRegistry::getComponentRecord(const ComponentId cid) const {
    return this->getElementById(cid);
}

[[nodiscard]] ComponentRecord& ComponentRegistry::getComponentRecord(const ComponentId cid) {
    return this->getElementById(cid);
}

EcsVec<ComponentId> ComponentRegistry::resolveRequiredComponentIds(const EcsVec<ComponentId>& directRequired) {
    EcsVec<ComponentId> ids = directRequired.clone();

    for (size_t i = 0; i < ids.size; ++i) {
        for (const ComponentId otherId : this->getComponentRecord(ids[i]).required) {
            if (!ids.has(otherId)) {
                ids.push_back(otherId);
            }
        }
    }

#ifndef NDEBUG
    std::vector<ComponentId> stack;
    std::vector<ComponentId> visited;

    auto contains = [](const std::vector<ComponentId>& values, const ComponentId id) {
        return std::ranges::any_of(values, [id](const ComponentId current) {
            return current == id;
        });
    };

    auto hasCycle = [&](auto&& self, const ComponentId id) -> bool {
        if (contains(visited, id)) {
            return false;
        }

        if (contains(stack, id)) {
            return true;
        }

        stack.push_back(id);

        for (const ComponentId otherId : this->getComponentRecord(id).required) {
            if (self(self, otherId)) {
                return true;
            }
        }

        stack.pop_back();
        visited.push_back(id);
        return false;
    };

    for (const ComponentId id : directRequired) {
        ecs_assert(!hasCycle(hasCycle, id), "cycle detected in required components");
    }
#endif

    return ids;
}
