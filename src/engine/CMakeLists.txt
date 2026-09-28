# KSEngine Qt-free core
cmake_minimum_required(VERSION 3.16)
project(ksengine VERSION 2.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KSENGINE_QT_FREE "Build without Qt" ON)

set(KSENGINE_QT_FREE_DIRS Math physics devices FileFormat archive Config network material sim)
set(KSENGINE_SOURCES "")
foreach(_dir ${KSENGINE_QT_FREE_DIRS})
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}")
    file(GLOB_RECURSE _dir_sources
      "${_dir}/*.h" "${_dir}/*.cpp" "${_dir}/*.c"
      "${_dir}/**/*.h" "${_dir}/**/*.cpp")
    list(APPEND KSENGINE_SOURCES ${_dir_sources})
  endif()
endforeach()

# Exclude ALL known Qt-heavy / editor-only paths
foreach(_pat
    "physics/weather/"
    "VehiclePhysics"
    "VehiclePhysicsModels"
    "TireCurveEditor"
    "PhysicsSimulations"
    "TrackPhysics"
    "PhysicsValidator"
    "TelemetryPhysics"
    "WeatherConfig"
    "ReplaySystem"
    "PhysicsMessage"
    "StrategySimulator"
    "phys_LapTimer"
    "devices/3dprint/"
    "devices/scanners/"
    "DeviceSettingsWidget"
    "MultiplayerWidget"
    "SimulatorServerApp"
    "PostProcessing"
    "ThreeDPrint"
    "GCodeGenerator"
    "PrintPreview"
    "SlicerEngine"
    "PrinterProfile"
    "TurntableScanner"
    "StructuredLight"
    "StereoVision"
    "Photogrammetry"
)
  list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "${_pat}")
endforeach()

list(APPEND KSENGINE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineModule.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine.h")

add_library(ksengine STATIC ${KSENGINE_SOURCES})
target_include_directories(ksengine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/physics
  ${CMAKE_CURRENT_SOURCE_DIR}/devices
  ${CMAKE_CURRENT_SOURCE_DIR}/sim)
target_compile_definitions(ksengine PUBLIC KSENGINE_QT_FREE=1)
message(STATUS "ksengine Qt-free sources filtered")
