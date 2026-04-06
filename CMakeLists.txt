cmake_minimum_required(VERSION 3.31)
project(untitled LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ECS_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer for ECS targets" OFF)

function(ecs_enable_sanitizers target_name)
    if (NOT ECS_ENABLE_SANITIZERS)
        return()
    endif ()

    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(WARNING "ECS_ENABLE_SANITIZERS is only supported with GCC or Clang")
        return()
    endif ()

    target_compile_options(${target_name} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -g
    )
    target_link_options(${target_name} PRIVATE
            -fsanitize=address,undefined
    )
endfunction()

add_library(ecs STATIC
        ecs/World.hpp
        ecs/Query.hpp
        ecs/System.hpp
        ecs/Commands.hpp
        ecs/Addons.hpp
        engine/Engine.hpp
        ecs/EntityRef.hpp
        ecs/EntityRef.cpp
        ecs/core/base/ComponentType.hpp
        ecs/core/base/EcsAssert.hpp
        ecs/core/base/EcsType.hpp
        ecs/core/base/TypeCounter.hpp
        ecs/core/base/typing.hpp
        ecs/core/memory/Allocator.hpp
        ecs/core/memory/Allocator.cpp
        ecs/core/storage/EcsVec.hpp
        ecs/core/storage/SparseSet.hpp
        ecs/core/storage/Table.hpp
        ecs/core/storage/Table.cpp
        ecs/core/storage/TableMap.hpp
        ecs/core/storage/TableRegistry.hpp
        ecs/core/storage/TableRegistry.cpp
        ecs/core/storage/TypedTable.hpp
        ecs/core/entity/EntityRegistry.hpp
        ecs/core/entity/EntityRegistry.cpp
        ecs/core/entity/EntityManager.hpp
        ecs/core/entity/EntityManager.cpp
        ecs/core/entity/EntityType.cpp
        ecs/core/component/ComponentRegistry.hpp
        ecs/core/component/ComponentRegistry.cpp
        ecs/core/query/BloomFilter.hpp
        ecs/core/query/Iter.hpp
        ecs/core/query/Iter.cpp
        ecs/core/query/Query.cpp
        ecs/core/query/QueryRegistry.hpp
        ecs/core/query/QueryRegistry.cpp
        ecs/core/query/QueryTables.hpp
        ecs/core/system/SystemDesc.hpp
        ecs/core/system/SystemRegistry.hpp
        ecs/core/system/SystemRegistry.cpp
        ecs/core/system/WorldAccess.hpp
        ecs/core/event/EventRegistry.hpp
        ecs/core/event/EntityEventRegistry.hpp
        ecs/core/relation/Relations.hpp
        ecs/core/plugin/PluginRegistry.hpp
        ecs/core/plugin/Reflect.hpp
        ecs/core/plugin/SingletonRegistry.hpp
        ecs/world/World.cpp
        ecs/command/Commands.cpp
        ecs/addon/Addons.cpp
)

target_include_directories(ecs PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/base
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/memory
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/storage
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/entity
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/component
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/query
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/system
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/event
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/relation
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/core/plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/world
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/command
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs/addon
)
target_precompile_headers(ecs PRIVATE ecs/pch.hpp)
set_target_properties(ecs PROPERTIES
        CXX_STANDARD 26
        CMAKE_CXX_STANDARD_REQUIRED ON
        CMAKE_CXX_EXTENSIONS OFF
)
ecs_enable_sanitizers(ecs)

add_executable(untitled main.cpp)
target_link_libraries(untitled PRIVATE ecs)
target_precompile_headers(untitled PRIVATE ecs/pch.hpp)
ecs_enable_sanitizers(untitled)

add_executable(ecs_bench_query
        benchmarks/query_bench.cpp
        benchmarks/BenchCommon.hpp)
target_link_libraries(ecs_bench_query PRIVATE ecs)
target_precompile_headers(ecs_bench_query PRIVATE ecs/pch.hpp)
ecs_enable_sanitizers(ecs_bench_query)

add_executable(ecs_bench_entity_component
        benchmarks/entity_component_bench.cpp
        benchmarks/BenchCommon.hpp)
target_link_libraries(ecs_bench_entity_component PRIVATE ecs)
target_precompile_headers(ecs_bench_entity_component PRIVATE ecs/pch.hpp)
ecs_enable_sanitizers(ecs_bench_entity_component)

include(CTest)
if (BUILD_TESTING)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CRITERION REQUIRED IMPORTED_TARGET criterion)

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )

    add_executable(ecs_tests ${TEST_SOURCES}
            tests/query_basic.cpp
            tests/system_basic.cpp
            tests/commands_basic.cpp
            tests/system_condition.cpp)
    target_link_libraries(ecs_tests PRIVATE ecs PkgConfig::CRITERION)
    target_precompile_headers(ecs_tests PRIVATE ecs/pch.hpp)
    ecs_enable_sanitizers(ecs_tests)
    add_test(NAME ecs_tests COMMAND ecs_tests)
endif ()
