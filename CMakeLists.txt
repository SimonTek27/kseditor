cmake_minimum_required(VERSION 3)

project(kseditor
    VERSION 1.16.4
    LANGUAGES C CXX
)

cmake_policy(VERSION 3.15)

# vcpkg auto-detection disabled due to CMake 4.2.0 compatibility issues
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows")
#     # OpenVDB uses FindOpenVDB.cmake (not a Config file)
#     if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows/share/openvdb/FindOpenVDB.cmake")
#         list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows/share/openvdb")
#     endif()
#     message(STATUS "vcpkg detected: ${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg")
# endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix for moc_predefs issue - only set for GCC/Clang, let CMake handle MSVC internally
if(NOT MSVC)
    set(CMAKE_CXX_COMPILER_PREDEFINES_COMMAND
        "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -dM -E"
    )
endif()

# Add MSVC specific flags required by Qt6
if(MSVC)
    add_compile_options(/permissive-)
endif()

# Auto-detect Qt6 path from environment or cache
if(NOT QT6_PATH)
    if(DEFINED ENV{QTDIR})
        set(QT6_PATH "$ENV{QTDIR}")
    elseif(DEFINED ENV{QT6_DIR})
        set(QT6_PATH "$ENV{QT6_DIR}")
    elseif(MSVC)
        set(QT6_PATH "C:/Qt/6.11.1/msvc2022_64" CACHE PATH "Qt6 path")
    else()
        set(QT6_PATH "C:/Qt/6.11.1/mingw_64" CACHE PATH "Qt6 path")
    endif()
endif()
if(QT6_PATH)
    list(PREPEND CMAKE_PREFIX_PATH "${QT6_PATH}")
endif()

# Auto-detect vcpkg path (append after Qt)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows")
endif()

set(QML_IMPORT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/qml CACHE PATH "QML import paths")

find_package(Qt6 COMPONENTS Core Gui Widgets Network Qml Quick QuickControls2 QuickWidgets Multimedia OpenGL REQUIRED)
find_package(Vulkan QUIET)


# ============================================================================
# Eigen 3.4+ (header-only linear algebra)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/eigen/Eigen/Core")
    set(HAS_EIGEN TRUE)
    message(STATUS "Eigen found: ${CMAKE_CURRENT_SOURCE_DIR}/external/eigen")
    add_compile_definitions(HAS_EIGEN=1)
else()
    set(HAS_EIGEN FALSE)
    message(STATUS "Eigen not found in external/eigen")
    add_compile_definitions(HAS_EIGEN=0)
endif()

# ============================================================================
# mikktspace 1.0+ (tangent space computation)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace/mikktspace.c")
    set(HAS_MIKKTSPACE TRUE)
    message(STATUS "mikktspace found: ${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace")
    add_library(mikktspace STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace/mikktspace.c"
    )
    target_include_directories(mikktspace PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace"
    )
    set_target_properties(mikktspace PROPERTIES
        C_STANDARD 11
        AUTOMOC OFF
    )
    add_compile_definitions(HAS_MIKKTSPACE=1)
else()
    set(HAS_MIKKTSPACE FALSE)
    message(STATUS "mikktspace not found in external/mikktspace")
    add_compile_definitions(HAS_MIKKTSPACE=0)
endif()

# ============================================================================
# Bullet Physics 3.25+ - DISABLED for now
set(HAS_BULLET FALSE)
