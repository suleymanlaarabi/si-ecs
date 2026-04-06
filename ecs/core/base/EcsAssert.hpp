#pragma once
#include <cstdio>
#include <cstdlib>

#ifndef NDEBUG
#define ecs_assert(condition, ...) \
    do { \
        if (!(condition)) { \
            std::fprintf(stderr, __VA_ARGS__); \
            std::fputc('\n', stderr); \
            std::abort(); \
        } \
    } while (false)
#else
#define ecs_assert(condition, ...)
#endif

#define ecs_assert_entity_is_valid(entity) ecs_assert(entity.index != 0, "is not a valid entity id")

#define ecs_assert_entity_alive(entity) \
    do { \
        ecs_assert_entity_is_valid(entity); \
        ecs_assert(this->isAlive(entity), "entity is not alive"); \
    } while (false)

#define ecs_assert_component_registered(cid) \
    ecs_assert(this->isComponentRegistered(cid), "component is not registered")
