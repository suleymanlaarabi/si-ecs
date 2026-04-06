#include <cstdlib>
#include <cstring>

#include "EcsType.hpp"

void EntityType::release() const {
    free(cids);
}

bool EntityType::operator==(const EntityType& other) const noexcept {
    if (count != other.count) {
        return false;
    }

    if (count == 0) {
        return true;
    }

    return std::memcmp(cids, other.cids, count * sizeof(ComponentId)) == 0;
}

#include <algorithm>
#include <cstdint>
#include <limits>

[[nodiscard]] uint16_t EntityType::findIndex(const ComponentId cid) const {
    if (constexpr uint16_t linearThreshold = 12; count <= linearThreshold) {
        for (uint16_t i = 0; i < count; ++i) {
            if (cids[i] == cid) {
                return i;
            }
        }
        return std::numeric_limits<uint16_t>::max();
    }

    uint16_t left = 0;
    uint16_t right = count;

    while (left < right) {
        if (const uint16_t mid = left + (right - left) / 2; cids[mid] < cid) {
            left = static_cast<uint16_t>(mid + 1);
        } else {
            right = mid;
        }
    }

    if (left < count && cids[left] == cid) {
        return left;
    }

    return std::numeric_limits<uint16_t>::max();
}

static uint16_t findInsertPos(const ComponentId* cids, uint16_t count, ComponentId cid) {
    uint16_t pos = 0;
    while (pos < count && cids[pos] < cid) {
        ++pos;
    }
    return pos;
}

static ComponentId* cloneCids(const ComponentId* src, uint16_t count) {
    if (count == 0) {
        return nullptr;
    }

    auto* dst = static_cast<ComponentId*>(malloc(sizeof(ComponentId) * count));
    if (src != nullptr) {
        memcpy(dst, src, sizeof(ComponentId) * count);
    }
    return dst;
}


void EntityType::add(const ComponentId cid) {
    const uint16_t insertPos = findInsertPos(cids, count, cid);

    if (insertPos < count && cids[insertPos] == cid) {
        return;
    }

    cids = static_cast<ComponentId*>(realloc(cids, sizeof(ComponentId) * (count + 1)));

    if (insertPos < count) {
        memmove(
            cids + insertPos + 1,
            cids + insertPos,
            (count - insertPos) * sizeof(ComponentId)
        );
    }

    cids[insertPos] = cid;
    ++count;
}

[[nodiscard]] EntityType EntityType::withAdd(const ComponentId cid) const {
    EntityType newType = {
        .cids = cloneCids(cids, count),
        .count = count
    };

    newType.add(cid);
    return newType;
}

[[nodiscard]] EntityType EntityType::clone() const {
    return {
        .cids = cloneCids(cids, count),
        .count = count
    };
}

[[nodiscard]] EntityType EntityType::withRemove(const ComponentId cid) const {
    if (count == 0) {
        return {
            .cids = nullptr,
            .count = 0
        };
    }

    const uint16_t removePos = findInsertPos(cids, count, cid);

    if (removePos == count || cids[removePos] != cid) {
        return {
            .cids = cloneCids(cids, count),
            .count = count
        };
    }

    const EntityType newType = {
        .cids = (count > 1)
                    ? static_cast<ComponentId*>(malloc(sizeof(ComponentId) * (count - 1)))
                    : nullptr,
        .count = static_cast<uint16_t>(count - 1)
    };

    if (removePos > 0) {
        memcpy(newType.cids, cids, removePos * sizeof(ComponentId));
    }

    if (removePos + 1 < count) {
        memcpy(
            newType.cids + removePos,
            cids + removePos + 1,
            (count - removePos - 1) * sizeof(ComponentId)
        );
    }

    return newType;
}

[[nodiscard]] ComponentId EntityType::min() const {
    if (count == 0) {
        return 0;
    }
    return cids[0];
}


[[nodiscard]] ComponentId EntityType::max() const {
    if (count == 0) {
        return 0;
    }
    return cids[this->count - 1];
}
