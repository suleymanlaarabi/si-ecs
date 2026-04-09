cmake_minimum_required(VERSION 3.31)
project(untitled LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
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

file(GLOB_RECURSE ECS_SOURCES CONFIGURE_DEPENDS
        "ecs/*.cpp"
        "ecs/*.hpp"
        "engine/*.cpp"
        "engine/*.hpp"
)

add_library(ecs STATIC ${ECS_SOURCES})

set(ECS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
foreach (FILE IN LISTS ECS_SOURCES)
    get_filename_component(DIR ${FILE} DIRECTORY)
    list(APPEND ECS_INCLUDE_DIRS ${DIR})
endforeach ()
list(REMOVE_DUPLICATES ECS_INCLUDE_DIRS)

target_include_directories(ecs PUBLIC
        ${ECS_INCLUDE_DIRS}
        $ENV{HOME}/.local/include
)

target_link_directories(ecs PUBLIC $ENV{HOME}/.local/lib)
target_link_libraries(ecs PUBLIC tcc)

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

include(CTest)
if (BUILD_TESTING)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CRITERION REQUIRED IMPORTED_TARGET criterion)

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
            "tests/*.cpp"
            "tests/*.hpp"
    )

    add_executable(ecs_tests ${TEST_SOURCES})
    target_link_libraries(ecs_tests PRIVATE ecs PkgConfig::CRITERION)
    target_precompile_headers(ecs_tests PRIVATE ecs/pch.hpp)
    ecs_enable_sanitizers(ecs_tests)
    add_test(NAME ecs_tests COMMAND ecs_tests)
endif ()
