cmake_minimum_required(VERSION 3.31)
project(untitled LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ecs STATIC
        ecs/ComponentRegistry.cpp
        ecs/EntityRegistry.cpp
        ecs/EntityManager.cpp
        ecs/Query.cpp
        ecs/Iter.cpp
        ecs/Table.cpp
        ecs/TableRegistry.cpp
        ecs/World.hpp
        ecs/EntityType.cpp
        ecs/World.cpp
        ecs/EventRegistry.hpp
        ecs/TypedTable.hpp
        ecs/EcsAssert.hpp
        ecs/SparseSet.hpp
        ecs/Query.hpp
        ecs/Iter.hpp
        ecs/EcsVec.hpp
        ecs/QueryRegistry.hpp
        ecs/QueryRegistry.cpp
        ecs/PluginRegistry.hpp
        ecs/SingletonRegistry.hpp
        ecs/SystemRegistry.hpp
        ecs/System.hpp
        ecs/typing.hpp
        ecs/SystemRegistry.cpp
        ecs/Commands.hpp
        ecs/Commands.cpp
        ecs/Addons.hpp
        ecs/Addons.cpp
        ecs/EntityEventRegistry.hpp
        ecs/TableMap.hpp
        engine/Engine.hpp
        ecs/EntityRef.hpp
        ecs/EntityRef.cpp
        ecs/Reflect.hpp
        ecs/BloomFilter.hpp
        ecs/Allocator.hpp
        ecs/Allocator.cpp
)

target_include_directories(ecs PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/ecs
)
target_precompile_headers(ecs PRIVATE ecs/pch.hpp)
set_target_properties(ecs PROPERTIES
        CXX_STANDARD 26
        CMAKE_CXX_STANDARD_REQUIRED ON
        CMAKE_CXX_EXTENSIONS OFF
)

add_executable(untitled main.cpp)
target_link_libraries(untitled PRIVATE ecs)
target_precompile_headers(untitled PRIVATE ecs/pch.hpp)

add_executable(ecs_bench_query
        benchmarks/query_bench.cpp
        benchmarks/BenchCommon.hpp)
target_link_libraries(ecs_bench_query PRIVATE ecs)
target_precompile_headers(ecs_bench_query PRIVATE ecs/pch.hpp)

add_executable(ecs_bench_entity_component
        benchmarks/entity_component_bench.cpp
        benchmarks/BenchCommon.hpp)
target_link_libraries(ecs_bench_entity_component PRIVATE ecs)
target_precompile_headers(ecs_bench_entity_component PRIVATE ecs/pch.hpp)

include(CTest)
if(BUILD_TESTING)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CRITERION REQUIRED IMPORTED_TARGET criterion)

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )

    add_executable(ecs_tests ${TEST_SOURCES}
            tests/query_basic.cpp
            tests/system_basic.cpp
            tests/commands_basic.cpp)
    target_link_libraries(ecs_tests PRIVATE ecs PkgConfig::CRITERION)
    target_precompile_headers(ecs_tests PRIVATE ecs/pch.hpp)
    add_test(NAME ecs_tests COMMAND ecs_tests)
endif()
