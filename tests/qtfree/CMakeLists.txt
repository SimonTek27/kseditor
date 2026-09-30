# Qt-free unit tests — CTest only (no Qt, no QTest, no kseditor_lib).
#
# Two ways to build:
#   from the repo root qt-free configure (enable_testing + add_subdirectory)
#   standalone: cmake -S tests/qtfree -B build_qtfree_tests
cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ks_qtfree_tests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    if(NOT TARGET ksengine)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../src/engine" ksengine)
    endif()
endif()

enable_testing()

set(KS_QT_FREE_TESTS
    ecs_test
    engine_tick_test
    math_test
    ai_line_test
    bank_test
    lua_test
    script_module_test
)

foreach(_t ${KS_QT_FREE_TESTS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_t}.cpp")
        add_executable(${_t} "${_t}.cpp")
        target_link_libraries(${_t} PRIVATE ksengine)
        add_test(NAME ${_t} COMMAND ${_t})
    endif()
endforeach()

# Parity 1.4 — determinism
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_determinism.cpp")
    add_executable(test_determinism test_determinism.cpp)
    target_link_libraries(test_determinism PRIVATE ksengine)
    add_test(NAME test_determinism COMMAND test_determinism)
endif()

# Parity 1.1 — golden trajectory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_PhysicsGolden.cpp")
    add_executable(test_PhysicsGolden test_PhysicsGolden.cpp)
    target_link_libraries(test_PhysicsGolden PRIVATE ksengine)
    # Working directory = repo root so tests/data/golden_lap.csv resolves
    add_test(NAME test_PhysicsGolden COMMAND test_PhysicsGolden
             WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../..")
endif()
