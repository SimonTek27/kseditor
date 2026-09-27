# ============================================================================
# KSEngine - Independent Engine Framework (Qt-free core)
# ============================================================================
# Policy: this target must NOT depend on Qt. Modules that still use Qt
# (Audio, Graphics/QVulkanWindow, mesh editors, Scripting UI, sys/LogManager,
# etc.) are EXCLUDED until ported. SimulatorApp links this core only.
# ============================================================================

cmake_minimum_required(VERSION 3.16)

project(ksengine
    VERSION 2.1.0
    LANGUAGES C CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# AUTOMOC intentionally OFF — engine is Qt-free

if(MSVC)
    add_compile_options(/permissive-)
    add_compile_definitions(NOMINMAX)
    add_compile_options(/FS)
    add_compile_options(/bigobj)
endif()

set(KSENGINE_QT_FREE_DIRS
    Math
    physics
    devices
    FileFormat
    archive
    Config
    network
    material
    terrain
    hwril
)

set(KSENGINE_SOURCES "")
foreach(_dir ${KSENGINE_QT_FREE_DIRS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}")
        file(GLOB_RECURSE _dir_sources
            "${_dir}/*.h"
            "${_dir}/*.cpp"
            "${_dir}/*.c"
            "${_dir}/**/*.h"
            "${_dir}/**/*.cpp"
            "${_dir}/**/*.c"
        )
        list(APPEND KSENGINE_SOURCES ${_dir_sources})
    endif()
endforeach()

list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/weather/WeatherEditor\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/weather/WeatherModule\\.")

# Physics: exclude modules still Qt-heavy
# INCLUDED (Qt-free): PhysicsEngine, Aero, Tires, Suspension, Diff, Gearbox,
# WeatherPhysics, Damage, BrakeThermal, ChassisSimulator, HybridSystem, Logger, Profiler
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/.*VehiclePhysics\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/.*VehiclePhysicsModels\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/.*TireCurveEditor\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/.*PhysicsSimulations\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/.*TrackPhysics\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/.*ACModelManager\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/.*BrakeWearSystem\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/.*CharacterPhysics\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "physics/weather/")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "VehiclePhysics\\.cpp$")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "VehiclePhysics\\.h$")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "TireCurveEditor\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "PhysicsSimulations\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "TrackPhysics\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "ACModelManager\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "BrakeWearSystem\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "CharacterPhysics\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "phys_")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "/phys_")

list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "devices/3dprint/")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "devices/scanners/")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "network/NetworkManager\\.cpp$")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "PhotogrammetryCapture\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "TurntableScanner\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "SlicerEngine\\.")
list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "SupportGenerator\\.")

list(APPEND KSENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/EngineModule.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine.h"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/GpuProfiler.cpp")
    list(APPEND KSENGINE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/GpuProfiler.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/GpuProfiler.h")
endif()

add_library(ksengine STATIC ${KSENGINE_SOURCES})

target_include_directories(ksengine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Math
    ${CMAKE_CURRENT_SOURCE_DIR}/physics
    ${CMAKE_CURRENT_SOURCE_DIR}/devices
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/openxr/openxr.h")
    target_include_directories(ksengine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/openxr")
    target_compile_definitions(ksengine PUBLIC HAS_OPENXR_HEADERS=1)
endif()

target_compile_definitions(ksengine PUBLIC
    KSASSETTOCORSA_EXPORTS
    KSENGINE_QT_FREE=1
)

list(LENGTH KSENGINE_SOURCES _src_count)
message(STATUS "ksengine (Qt-free): ${_src_count} source files from ${KSENGINE_QT_FREE_DIRS}")
message(STATUS "Excluded until ported: ACModelManager, BrakeWearSystem, CharacterPhysics, Audio, Graphics, mesh editors, Scripting")
