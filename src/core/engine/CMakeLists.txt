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
set(CMAKE_AUTOMOC ON)
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

file(GLOB_RECURSE KSENGINE_MATH_SOURCES
    "Math/*.h"
    "Math/*.cpp"
)

file(GLOB_RECURSE KSENGINE_PHYSICS_SOURCES
    "Physics/*.h"
    "Physics/*.cpp"
)

file(GLOB_RECURSE KSENGINE_GRAPHICS_SOURCES
    "Graphics/*.h"
    "Graphics/*.cpp"
    "Graphics/**/*.h"
    "Graphics/**/*.cpp"
)

file(GLOB_RECURSE KSENGINE_AUDIO_SOURCES
    "Audio/*.h"
    "Audio/*.cpp"
)

file(GLOB_RECURSE KSENGINE_ANIMATION_SOURCES
    "Animation/*.h"
    "Animation/*.cpp"
)

file(GLOB_RECURSE KSENGINE_MESH_SOURCES
    "Mesh/*.h"
    "Mesh/*.cpp"
)

file(GLOB_RECURSE KSENGINE_MATERIAL_SOURCES
    "Material/*.h"
    "Material/*.cpp"
)

file(GLOB_RECURSE KSENGINE_SCRIPTING_SOURCES
    "Scripting/*.h"
    "Scripting/*.cpp"
    "Scripting/**/*.h"
    "Scripting/**/*.cpp"
)

file(GLOB_RECURSE KSENGINE_FILEFORMAT_SOURCES
    "FileFormat/*.h"
    "FileFormat/*.cpp"
)

file(GLOB_RECURSE KSENGINE_ASSETS_SOURCES
    "Assets/*.h"
    "Assets/*.cpp"
)

file(GLOB_RECURSE KSENGINE_DEVICES_SOURCES
    "Devices/*.h"
    "Devices/*.cpp"
    "Devices/**/*.h"
    "Devices/**/*.cpp"
)

file(GLOB_RECURSE KSENGINE_TOOLS_SOURCES
    "Tools/*.h"
    "Tools/*.cpp"
)

file(GLOB_RECURSE KSENGINE_SYS_SOURCES
    "Sys/*.h"
    "Sys/*.cpp"
)

file(GLOB_RECURSE KSENGINE_ARCHIVE_SOURCES
    "Archive/*.h"
    "Archive/*.cpp"
)

file(GLOB_RECURSE KSENGINE_NETWORK_SOURCES
    "Network/*.h"
    "Network/*.cpp"
)

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
    ${KSENGINE_SYS_SOURCES}
    ${KSENGINE_ARCHIVE_SOURCES}
    ${KSENGINE_NETWORK_SOURCES}
    "EngineModule.h"
)

target_include_directories(ksengine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# Backward-compatible include paths so files moved into engine/ can still
# resolve old core/..., modules/..., plugins/... references
target_include_directories(ksengine PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/core"
)

# ============================================================================
# Dependencies
# ============================================================================

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

# Vulkan
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    target_include_directories(ksengine PUBLIC ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(ksengine PUBLIC ${Vulkan_LIBRARIES})
    target_compile_definitions(ksengine PUBLIC HAS_VULKAN=1)
endif()

# Bullet Physics
find_package(Bullet QUIET)
if(BULLET_FOUND OR TARGET BulletDynamics)
    target_link_libraries(ksengine PUBLIC Bullet3Common BulletDynamics BulletCollision LinearMath)
    target_compile_definitions(ksengine PUBLIC HAS_BULLET=1)
endif()

# Eigen
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/eigen/Eigen/Core")
    target_include_directories(ksengine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/eigen")
    target_compile_definitions(ksengine PUBLIC HAS_EIGEN=1)
endif()

# mikktspace
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/mikktspace/mikktspace.c")
    if(NOT TARGET mikktspace)
        add_library(mikktspace STATIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/mikktspace/mikktspace.c")
        target_include_directories(mikktspace PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/mikktspace")
        set_target_properties(mikktspace PROPERTIES C_STANDARD 11 AUTOMOC OFF)
    endif()
    target_link_libraries(ksengine PUBLIC mikktspace)
    target_compile_definitions(ksengine PUBLIC HAS_MIKKTSPACE=1)
    # Ensure the engine itself can find mikktspace.h
    target_include_directories(ksengine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/mikktspace")
else()
    target_compile_definitions(ksengine PUBLIC HAS_MIKKTSPACE=0)
endif()

# Lua
find_package(Lua QUIET)
if(Lua_FOUND)
    target_link_libraries(ksengine PUBLIC ${LUA_LIBRARIES})
    target_include_directories(ksengine PUBLIC ${LUA_INCLUDE_DIR})
    target_compile_definitions(ksengine PUBLIC HAS_LUA=1)
endif()

# Python
find_package(Python3 COMPONENTS Development QUIET)
if(Python3_FOUND)
    target_link_libraries(ksengine PUBLIC ${Python3_LIBRARIES})
    target_include_directories(ksengine PUBLIC ${Python3_INCLUDE_DIRS})
    target_compile_definitions(ksengine PUBLIC HAS_PYTHON=1)
endif()

# zlib
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    target_link_libraries(ksengine PUBLIC ZLIB::ZLIB)
    target_compile_definitions(ksengine PUBLIC HAS_ZLIB=1)
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

# libigl
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/libigl/include/igl/boundary_loop.h")
    target_include_directories(ksengine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/libigl/include")
    target_compile_definitions(ksengine PUBLIC HAS_LIBIGL=1)
endif()

# ============================================================================
# Compile definitions
# ============================================================================

target_compile_definitions(ksengine PUBLIC
    KSASSETTOCORSA_EXPORTS
    KSASSETTOCORSAEVO_EXPORTS
)

message(STATUS "KSEngine v${PROJECT_VERSION} configured successfully")
