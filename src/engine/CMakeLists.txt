# ============================================================================
# KSEngine - Independent Engine Framework
# ============================================================================
# This is the core engine library, independent of the editor UI.
# It contains: Math, Physics, Graphics, Audio, Animation, Mesh, Material,
# Scripting, FileFormat, Assets, Devices, Tools, Sys, Archive, Network
# ============================================================================

cmake_minimum_required(VERSION 3.16)

project(ksengine
    VERSION 2.1.0
    LANGUAGES C CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# KSENGINE_NO_QT: Build engine without Qt dependency (for standalone simulator)
option(KSENGINE_NO_QT "Build ksengine without Qt6 dependency" OFF)

if(KSENGINE_NO_QT)
    message(STATUS "ksengine: Building WITHOUT Qt (KSENGINE_NO_QT=ON)")
    set(CMAKE_AUTOMOC OFF)
else()
    set(CMAKE_AUTOMOC ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC flags
if(MSVC)
    add_compile_options(/permissive-)
    add_compile_definitions(NOMINMAX)
    add_compile_options(/FS)
    add_compile_options(/bigobj)
endif()

# ============================================================================
# Collect engine source files
# ============================================================================

if(KSENGINE_NO_QT)
    # Qt-free mode: use explicit source lists instead of GLOB_RECURSE
    # to avoid Qt dependencies in many modules
    
    # Math - no Qt dependencies
    set(KSENGINE_MATH_SOURCES
        "Math/Vector2.h"
        "Math/Vector3.h"
        "Math/Vector4.h"
        "Math/Matrix4x4.h"
        "Math/Quaternion.h"
    )
    
    # Physics - exclude WeatherUI files
    file(GLOB_RECURSE _physics_tmp
        "physics/*.h" "physics/*.cpp"
    )
    list(FILTER _physics_tmp EXCLUDE REGEX "physics/weather/")
    set(KSENGINE_PHYSICS_SOURCES ${_physics_tmp})
    
    # Graphics - use explicit list, exclude Qt-dependent modules
    file(GLOB_RECURSE _graphics_tmp
        "Graphics/*.h" "Graphics/*.cpp" "Graphics/**/*.h" "Graphics/**/*.cpp"
    )
    list(_graphics_tmp EXCLUDE REGEX "Audio")
    list(_graphics_tmp EXCLUDE REGEX "animation")
    set(KSENGINE_GRAPHICS_SOURCES ${_graphics_tmp})
    
    # Audio - stubbed for NO_QT mode
    set(KSENGINE_AUDIO_SOURCES
        "Audio/AudioModuleBridge.h"
        "Audio/AudioModuleBridge.cpp"
    )
    
    # Animation - stubbed for NO_QT mode
    set(KSENGINE_ANIMATION_SOURCES
        "animation/AnimationTimeline.h"
        "animation/AnimationTimeline.cpp"
    )
    
    # Other modules with explicit lists (no GLOB_RECURSE)
    file(GLOB_RECURSE KSENGINE_MESH_SOURCES "mesh/*.h" "mesh/*.cpp")
    file(GLOB_RECURSE KSENGINE_MATERIAL_SOURCES "material/*.h" "material/*.cpp")
    file(GLOB_RECURSE KSENGINE_FILEFORMAT_SOURCES "FileFormat/*.h" "FileFormat/*.cpp")
    file(GLOB_RECURSE KSENGINE_ASSETS_SOURCES "assets/*.h" "assets/*.cpp")
    file(GLOB_RECURSE KSENGINE_DEVICES_SOURCES "devices/*.h" "devices/*.cpp")
    file(GLOB_RECURSE KSENGINE_TOOLS_SOURCES "Tools/*.h" "Tools/*.cpp")
    file(GLOB_RECURSE KSENGINE_VIDEO_SOURCES "Video/*.h" "Video/*.cpp")
    file(GLOB_RECURSE KSENGINE_SYS_SOURCES "sys/*.h" "sys/*.cpp")
    file(GLOB_RECURSE KSENGINE_ARCHIVE_SOURCES "archive/*.h" "archive/*.cpp")
    file(GLOB_RECURSE KSENGINE_CONFIG_SOURCES "Config/*.h" "Config/*.cpp")
    file(GLOB_RECURSE KSENGINE_NETWORK_SOURCES "network/*.h" "network/*.cpp")
endif()

# Qt6 mode source collection (outside the NO_QT if block)
file(GLOB_RECURSE KSENGINE_FILEFORMAT_SOURCES
    "FileFormat/*.h"
    "FileFormat/*.cpp"
)

file(GLOB_RECURSE KSENGINE_ASSETS_SOURCES
    "assets/*.h"
    "assets/*.cpp"
)
# FormatConverter: re-included (acFiles/ include path added below)
# Add acFiles KN5 parser sources needed by FormatConverter
set(_AC_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk/kseditor/plugins/simulators/kunos/assettocorsa/acFiles")
if(EXISTS "${_AC_FILES_DIR}/KN5Parser.cpp")
    list(APPEND KSENGINE_ASSETS_SOURCES
        "${_AC_FILES_DIR}/KN5Parser.cpp"
        "${_AC_FILES_DIR}/KN5Decrypt.cpp"
    )
endif()

file(GLOB_RECURSE KSENGINE_DEVICES_SOURCES
    "devices/*.h"
    "devices/*.cpp"
    "devices/**/*.h"
    "devices/**/*.cpp"
)
# Scanner files and 3D print files re-included

file(GLOB_RECURSE KSENGINE_TOOLS_SOURCES
    "Tools/*.h"
    "Tools/*.cpp"
)

file(GLOB_RECURSE KSENGINE_VIDEO_SOURCES
    "Video/*.h"
    "Video/*.cpp"
)

file(GLOB_RECURSE KSENGINE_SYS_SOURCES
    "sys/*.h"
    "sys/*.cpp"
)
# ModuleManager excluded (depends on many SDK editor modules)
list(FILTER KSENGINE_SYS_SOURCES EXCLUDE REGEX "sys/ModuleManager\\.cpp$")
# PluginManager excluded (depends on SDK plugins/base/ headers outside engine tree)
list(FILTER KSENGINE_SYS_SOURCES EXCLUDE REGEX "sys/PluginManager\\.cpp$")

file(GLOB_RECURSE KSENGINE_ARCHIVE_SOURCES
    "archive/*.h"
    "archive/*.cpp"
)

file(GLOB_RECURSE KSENGINE_CONFIG_SOURCES
    "Config/*.h"
    "Config/*.cpp"
)

file(GLOB_RECURSE KSENGINE_NETWORK_SOURCES
    "network/*.h"
    "network/*.cpp"
)
# Exclude NetworkManager.cpp - depends on yojimbo/ksnet
list(FILTER KSENGINE_NETWORK_SOURCES EXCLUDE REGEX "NetworkManager\\.cpp$")

# ============================================================================
# Skip AUTOMOC for headers whose .cpp files are excluded or missing
# ============================================================================

if(KSENGINE_NO_QT)
    # No Qt: skip AUTOMOC headers, MOC wrap, and Qt linkage
    set(_SKIP_MOC_HEADERS "")
    set(_EXPLICIT_MOC_SOURCES "")
else()
    # Skip AUTOMOC for headers whose .cpp files are excluded or missing
    set(_SKIP_MOC_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/devices/scanners/photogrammetry/PhotogrammetryCapture.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/devices/scanners/turntable_scanner/TurntableScanner.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/network/NetworkManager.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Audio/AudioModuleBridge.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Audio/AudioVSTHost.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Audio/AudioVST3Host.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Audio/AudioCore.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/mesh/Viewport3DSystem.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/sys/PluginManager.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/sys/ModuleManager.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/animation/AnimationTimeline.h"
    )
    set_source_files_properties(${_SKIP_MOC_HEADERS} PROPERTIES SKIP_AUTOMOC ON)

    # Explicit MOC for headers that AUTOMOC silently skips
    qt6_wrap_cpp(_EXPLICIT_MOC_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/devices/3dprint/SlicerEngine.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/devices/3dprint/SupportGenerator.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/FormatConverter.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/devices/xinput/XInputDevice.h"
    )
endif()

# ============================================================================
# Create engine library
# ============================================================================

add_library(ksengine STATIC
    ${KSENGINE_MATH_SOURCES}
    ${KSENGINE_PHYSICS_SOURCES}
    ${KSENGINE_GRAPHICS_SOURCES}
    ${KSENGINE_AUDIO_SOURCES}
    ${KSENGINE_ANIMATION_SOURCES}
    ${KSENGINE_MESH_SOURCES}
    ${KSENGINE_MATERIAL_SOURCES}
    ${KSENGINE_SCRIPTING_SOURCES}
    ${KSENGINE_FILEFORMAT_SOURCES}
    ${KSENGINE_ASSETS_SOURCES}
    ${KSENGINE_DEVICES_SOURCES}
    ${KSENGINE_TOOLS_SOURCES}
    ${KSENGINE_VIDEO_SOURCES}
    ${KSENGINE_SYS_SOURCES}
    ${KSENGINE_ARCHIVE_SOURCES}
    ${KSENGINE_CONFIG_SOURCES}
    ${KSENGINE_NETWORK_SOURCES}
    ${_EXPLICIT_MOC_SOURCES}
    "EngineModule.h"
)

target_include_directories(ksengine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# Backward-compatible include paths
# CMAKE_CURRENT_SOURCE_DIR = src/engine, so ../.. = kseditor/src
target_include_directories(ksengine PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

# ============================================================================
# Dependencies
# ============================================================================

if(KSENGINE_NO_QT)
    # Qt-free mode: use fake Qt headers from qt_compat/ and define KSENGINE_NO_QT
    # Force-include NoQtCompat.h before all headers to provide std alternatives
    target_include_directories(ksengine PRIVATE BEFORE
        "${CMAKE_CURRENT_SOURCE_DIR}/qt_compat"
    )
    target_compile_definitions(ksengine PUBLIC KSENGINE_NO_QT)
    target_compile_options(ksengine PRIVATE /FI"${CMAKE_CURRENT_SOURCE_DIR}/NoQtCompat.h")
else()
    # Qt6
    find_package(Qt6 COMPONENTS Core Gui Widgets Network Qml Quick Quick3D QuickControls2 QuickWidgets Multimedia OpenGL Sql Charts WebSockets Svg DataVisualization QUIET)

    if(Qt6_FOUND)
        target_link_libraries(ksengine PUBLIC
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Network
            Qt6::Qml
            Qt6::Quick
            Qt6::Quick3D
            Qt6::QuickControls2
            Qt6::QuickWidgets
            Qt6::Multimedia
            Qt6::OpenGL
            Qt6::Sql
            Qt6::Charts
            Qt6::WebSockets
        )
    endif()
endif()

# Vulkan
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    target_include_directories(ksengine PUBLIC ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(ksengine PUBLIC ${Vulkan_LIBRARIES})
    target_compile_definitions(ksengine PUBLIC HAS_VULKAN=1)
endif()

# acFiles (Assetto Corsa KN5 parser — needed by FormatConverter)
set(_AC_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk/kseditor/plugins/simulators/kunos/assettocorsa")
if(EXISTS "${_AC_FILES_DIR}/acFiles/KN5Types.h")
    target_include_directories(ksengine PRIVATE "${_AC_FILES_DIR}")
    target_compile_definitions(ksengine PRIVATE HAS_ACFILES=1)
endif()

# SDK include path (for PhotogrammetryCapture/TurntableScanner that reference VulkanViewportItem)
set(_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk/kseditor/modules")
if(EXISTS "${_SDK_DIR}/modellingEditor/VulkanViewportItem.h")
    target_include_directories(ksengine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()
# Note: HAS_SDK is NOT defined - ImportExport3D is not compiled into engine.
# ThreeDPrintQmlBridge.cpp uses #ifdef HAS_SDK to guard ImportExport3D calls.

# XInput (Windows gamepad support)
if(WIN32)
    target_link_libraries(ksengine PRIVATE xinput)
endif()

# Bullet Physics
find_package(Bullet QUIET)
if(BULLET_FOUND OR TARGET BulletDynamics)
    target_link_libraries(ksengine PUBLIC Bullet3Common BulletDynamics BulletCollision LinearMath)
    target_compile_definitions(ksengine PUBLIC HAS_BULLET=1)
endif()

# Eigen
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../external/eigen/Eigen/Core")
    target_include_directories(ksengine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../external/eigen")
    target_compile_definitions(ksengine PUBLIC HAS_EIGEN=1)
endif()

# mikktspace (vendored)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace/mikktspace.c")
    if(NOT TARGET mikktspace)
        add_library(mikktspace STATIC "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace/mikktspace.c")
        target_include_directories(mikktspace PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace")
        set_target_properties(mikktspace PROPERTIES C_STANDARD 11 AUTOMOC OFF)
    endif()
    target_link_libraries(ksengine PUBLIC mikktspace)
    target_compile_definitions(ksengine PUBLIC HAS_MIKKTSPACE=1)
else()
    target_compile_definitions(ksengine PUBLIC HAS_MIKKTSPACE=0)
endif()

# stb_vorbis (vendored)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/stb/stb_vorbis.c")
    target_include_directories(ksengine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/stb")
    target_compile_definitions(ksengine PUBLIC HAS_STB=1)
else()
    target_compile_definitions(ksengine PUBLIC HAS_STB=0)
endif()

# OpenXR headers (vendored)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/openxr/openxr.h")
    target_include_directories(ksengine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external")
    target_compile_definitions(ksengine PUBLIC HAS_OPENXR=1)
else()
    target_compile_definitions(ksengine PUBLIC HAS_OPENXR=0)
endif()

# Lua
find_package(Lua QUIET)
if(Lua_FOUND)
    target_link_libraries(ksengine PUBLIC ${LUA_LIBRARIES})
    target_include_directories(ksengine PUBLIC ${LUA_INCLUDE_DIR})
    target_compile_definitions(ksengine PUBLIC HAS_LUA=1)
endif()

# Python (disabled for now — debug libs often missing, simulator doesn't need it)
# find_package(Python3 COMPONENTS Development QUIET)
# if(Python3_FOUND AND Python3_LIBRARIES)
#     foreach(_pylib IN LISTS Python3_LIBRARIES)
#         if(NOT _pylib MATCHES "_d\\.lib$")
#             list(APPEND _pylib_release ${_pylib})
#         endif()
#     endforeach()
#     if(_pylib_release)
#         target_link_libraries(ksengine PRIVATE ${_pylib_release})
#         target_include_directories(ksengine PRIVATE ${Python3_INCLUDE_DIRS})
#         target_compile_definitions(ksengine PRIVATE HAS_PYTHON=1)
#     endif()
# endif()

# zlib
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    target_link_libraries(ksengine PUBLIC ZLIB::ZLIB)
    target_compile_definitions(ksengine PUBLIC HAS_ZLIB=1)
endif()

# FFmpeg (video encoding)
if(HAS_FFMPEG)
    if(FFMPEG_LIBRARIES)
        target_link_libraries(ksengine PUBLIC ${FFMPEG_LIBRARIES})
        target_include_directories(ksengine PUBLIC ${FFMPEG_INCLUDE_DIRS})
    elseif(TARGET PkgConfig::AVCODEC)
        target_link_libraries(ksengine PUBLIC PkgConfig::AVCODEC PkgConfig::AVFORMAT PkgConfig::SWSCALE PkgConfig::AVUTIL)
    endif()
    target_compile_definitions(ksengine PUBLIC HAS_FFMPEG=1)
endif()

# yojimbo (ksnet)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Network/ksnet/CMakeLists.txt")
    if(NOT TARGET Yojimbo::yojimbo)
        add_subdirectory(Network/ksnet ${CMAKE_BINARY_DIR}/ksnet EXCLUDE_FROM_ALL)
    endif()
    target_link_libraries(ksengine PUBLIC Yojimbo::yojimbo)
    target_include_directories(ksengine PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Network/ksnet/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Network/ksnet/serialize")
    target_compile_definitions(ksengine PUBLIC HAS_YOJIMBO=1)
endif()

# ImGui
find_package(imgui CONFIG QUIET)
if(imgui_FOUND)
    target_link_libraries(ksengine PUBLIC imgui::imgui)
    target_compile_definitions(ksengine PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD HAS_IMGUI=1)
endif()

# ============================================================================
# System DLL Integration - Static Linking
# ============================================================================
# Import libraries generated from DLLs in bin/simulator/system/
set(KSENGINE_IMPORT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/import_libs")
set(KSENGINE_SYSTEM_DLL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/simulator/system")

# FMOD Core (fmod64.dll) - Static linking via import library
if(EXISTS "${KSENGINE_IMPORT_LIB_DIR}/fmod64.lib")
    set(HAS_FMOD_RUNTIME TRUE)
    message(STATUS "FMOD Runtime: fmod64.lib found - linking statically")
    target_link_libraries(ksengine PUBLIC "${KSENGINE_IMPORT_LIB_DIR}/fmod64.lib")
    target_compile_definitions(ksengine PUBLIC HAS_FMOD_RUNTIME=1)
else()
    set(HAS_FMOD_RUNTIME FALSE)
    message(STATUS "FMOD Runtime: fmod64.lib NOT found")
    target_compile_definitions(ksengine PUBLIC HAS_FMOD_RUNTIME=0)
endif()

# FMOD Studio (fmodstudio64.dll) - Static linking via import library
if(EXISTS "${KSENGINE_IMPORT_LIB_DIR}/fmodstudio64.lib")
    set(HAS_FMOD_STUDIO TRUE)
    message(STATUS "FMOD Studio: fmodstudio64.lib found - linking statically")
    target_link_libraries(ksengine PUBLIC "${KSENGINE_IMPORT_LIB_DIR}/fmodstudio64.lib")
    target_compile_definitions(ksengine PUBLIC HAS_FMOD_STUDIO=1)
else()
    set(HAS_FMOD_STUDIO FALSE)
    message(STATUS "FMOD Studio: fmodstudio64.lib NOT found")
    target_compile_definitions(ksengine PUBLIC HAS_FMOD_STUDIO=0)
endif()

# Streamline (sl.common.dll) - Static linking via import library
if(EXISTS "${KSENGINE_IMPORT_LIB_DIR}/sl.common.lib")
    set(HAS_STREAMLINE TRUE)
    message(STATUS "Streamline: sl.common.lib found - linking statically")
    target_link_libraries(ksengine PUBLIC "${KSENGINE_IMPORT_LIB_DIR}/sl.common.lib")
    target_compile_definitions(ksengine PUBLIC HAS_STREAMLINE=1)
else()
    set(HAS_STREAMLINE FALSE)
    message(STATUS "Streamline: sl.common.lib NOT found")
    target_compile_definitions(ksengine PUBLIC HAS_STREAMLINE=0)
endif()

# VideoModes (ksengineVideoModes.dll) - Static linking via import library
if(EXISTS "${KSENGINE_IMPORT_LIB_DIR}/ksengineVideoModes.lib")
    set(HAS_VIDEOMODES TRUE)
    message(STATUS "VideoModes: ksengineVideoModes.lib found - linking statically")
    target_link_libraries(ksengine PUBLIC "${KSENGINE_IMPORT_LIB_DIR}/ksengineVideoModes.lib")
    target_compile_definitions(ksengine PUBLIC HAS_VIDEOMODES=1)
else()
    set(HAS_VIDEOMODES FALSE)
    message(STATUS "VideoModes: ksengineVideoModes.lib NOT found")
    target_compile_definitions(ksengine PUBLIC HAS_VIDEOMODES=0)
endif()

# Copy system DLLs to build output directory (for runtime deployment)
if(EXISTS "${KSENGINE_SYSTEM_DLL_DIR}")
    file(GLOB KSENGINE_SYSTEM_DLLS "${KSENGINE_SYSTEM_DLL_DIR}/*.dll")
    if(KSENGINE_SYSTEM_DLLS)
        add_custom_command(TARGET ksengine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/system"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${KSENGINE_SYSTEM_DLLS}
                "${CMAKE_BINARY_DIR}/bin/system/"
            COMMENT "Copying system DLLs to build output (runtime deployment)"
        )
        message(STATUS "Will copy system DLLs to build output for runtime")
    endif()

    # Copy raceflags assets
    if(EXISTS "${KSENGINE_SYSTEM_DLL_DIR}/raceflags")
        add_custom_command(TARGET ksengine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/system/raceflags"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${KSENGINE_SYSTEM_DLL_DIR}/raceflags"
                "${CMAKE_BINARY_DIR}/bin/system/raceflags"
            COMMENT "Copying raceflags assets to build output"
        )
        message(STATUS "Will copy raceflags assets to build output")
    endif()

    target_compile_definitions(ksengine PUBLIC
        KSENGINE_SYSTEM_DLL_DIR="${KSENGINE_SYSTEM_DLL_DIR}"
    )
endif()

# ============================================================================
# Compile definitions
# ============================================================================

target_compile_definitions(ksengine PUBLIC
    KSASSETTOCORSA_EXPORTS
    KSASSETTOCORSAEVO_EXPORTS
)

message(STATUS "KSEngine v${PROJECT_VERSION} configured successfully")
