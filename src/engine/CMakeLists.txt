# KSEngine — generic open-source sim core (Qt-free)
cmake_minimum_required(VERSION 3.16)
project(ksengine VERSION 2.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KSENGINE_QT_FREE "Build without Qt" ON)

# Explicitly refuse Qt for this target when Qt-free mode is on.
if(KSENGINE_QT_FREE)
  if(TARGET Qt6::Core OR TARGET Qt5::Core)
    message(WARNING "ksengine: Qt targets exist in the project but ksengine will not link them")
  endif()
endif()

set(KSENGINE_QT_FREE_DIRS
  Math physics devices FileFormat archive Config network material sim Graphics
  Audio assets sys AI animation hwril mesh Tools Scripting Video
)
set(KSENGINE_SOURCES "")
foreach(_dir ${KSENGINE_QT_FREE_DIRS})
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}")
    file(GLOB_RECURSE _dir_sources
      "${_dir}/*.h" "${_dir}/*.cpp" "${_dir}/*.c"
      "${_dir}/**/*.h" "${_dir}/**/*.cpp")
    list(APPEND KSENGINE_SOURCES ${_dir_sources})
  endif()
endforeach()

foreach(_pat
    "physics/weather/"
    "VehiclePhysics"
    "VehiclePhysicsModels"
    "TireCurveEditor"
    "PhysicsSimulations"
    "TrackPhysics"
    "PhysicsValidator"
    "devices/3dprint/"
    "devices/scanners/"
    "SimulatorApp"
    "GameMenuOverlay"
    "README.md"
    "StreamlineFunctions"
    "ksAssettocorsasndeventdefs"
    "CspConfigParser"
    "ACGuidsParser"
    "FSPROImporter"
    "FSPROExporter"
    "P3DModelLoader"
    "PAATextureConverter"
    "SevenZip"
    "VideoModesFunctions"
    "Blueprint/"
    "ScriptDebugger"
    "ShapeKeyModifier"
    "AssetPreviewWidget"
    "\\.ui$"
    "\\.qrc$"
)
  list(FILTER KSENGINE_SOURCES EXCLUDE REGEX "${_pat}")
endforeach()

list(APPEND KSENGINE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineModule.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/KsQtFreeGuard.h")

add_library(ksengine STATIC ${KSENGINE_SOURCES})
target_include_directories(ksengine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/physics
  ${CMAKE_CURRENT_SOURCE_DIR}/devices
  ${CMAKE_CURRENT_SOURCE_DIR}/sim
  ${CMAKE_CURRENT_SOURCE_DIR}/Graphics
  ${CMAKE_CURRENT_SOURCE_DIR}/Audio
  ${CMAKE_CURRENT_SOURCE_DIR}/assets
  ${CMAKE_CURRENT_SOURCE_DIR}/sys
  ${CMAKE_CURRENT_SOURCE_DIR}/mesh
  ${CMAKE_CURRENT_SOURCE_DIR}/Tools
  ${CMAKE_CURRENT_SOURCE_DIR}/Math)

if(KSENGINE_QT_FREE)
  target_compile_definitions(ksengine PUBLIC KSENGINE_QT_FREE=1)
  # Do not link Qt under any name.
  set_property(TARGET ksengine PROPERTY AUTOMOC OFF)
  set_property(TARGET ksengine PROPERTY AUTOUIC OFF)
  set_property(TARGET ksengine PROPERTY AUTORCC OFF)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(ksengine PUBLIC ${CMAKE_DL_LIBS})
endif()

message(STATUS "ksengine: generic sim core (KSENGINE_QT_FREE=${KSENGINE_QT_FREE})")
