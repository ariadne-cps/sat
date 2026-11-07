cmake_minimum_required(VERSION 3.15)

project(Sat VERSION 1.0)

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)

if(NOT TARGET ariadne-build)
    find_package(Git REQUIRED)
    # Keep the initial Build checkout output in English.
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "LC_ALL=C"
                "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}"
                submodule update --init -- ss/bui
        RESULT_VARIABLE _result)
    if(_result)
        message(FATAL_ERROR "Cannot initialize build submodule")
    endif()
    include("${CMAKE_CURRENT_SOURCE_DIR}/ss/bui/cmake/ProjectOptions.cmake")
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Pinned child commits guarantee their internal consistency.
    # Preserve explicit comparisons across independent dependency branches.
    require_same_dependency_commit(Build CHAINS
        "build" "bdd|build" "cdcl|build" "testing|build")
    require_same_dependency_commit(Threading CHAINS
        "bdd|threading" "cdcl|threading")
    require_same_dependency_commit(Testing CHAINS
        "testing" "bdd|testing" "cdcl|testing")
endif()

setup_project()

include_directories("${PROJECT_SOURCE_DIR}/include")

add_subdirectory(ss)

add_library(sat INTERFACE)
target_include_directories(sat INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/ariadne-sat>
)
target_link_libraries(sat INTERFACE bdd cdcl)

add_subdirectory(src)
target_link_libraries(SAT_SRC PRIVATE sat)

ariadne_register_public_headers(
    TARGET sat
    DIRECTORIES "${PROJECT_SOURCE_DIR}/include/sat"
    FILES "${PROJECT_SOURCE_DIR}/include/ariadne-sat.hpp"
)

setup_project_library(
    TARGET ariadne-sat
    OBJECTS SAT_SRC
    LINK_LIBRARIES sat
)

setup_standalone_project_tests(
    TARGET ariadne-sat
    TEST_DIRECTORY tests
    TEST_TARGET tests
    SOURCES
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/src"
)

if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    ariadne_install_dependency_bundle(
        TARGET sat
        DESTINATION include/ariadne-sat
    )
endif()
