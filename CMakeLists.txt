cmake_minimum_required(VERSION 3.16)

project(kseditor
    VERSION 1.16.4
    LANGUAGES C CXX
)

# Auto-detect vcpkg if present
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows")
    # OpenVDB uses FindOpenVDB.cmake (not a Config file)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows/share/openvdb/FindOpenVDB.cmake")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows/share/openvdb")
    endif()
    message(STATUS "vcpkg detected: ${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg")
endif()

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

find_package(Qt6 COMPONENTS Core Gui Widgets Network Qml Quick Quick3D QuickControls2 QuickWidgets Multimedia OpenGL Sql Charts WebSockets REQUIRED)
find_package(Vulkan QUIET)
find_package(Qt6 COMPONENTS 3DCore 3DRender 3DExtras 3DLogic 3DInput 3DAnimation QUIET)

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
# Bullet Physics 3.25+ - DISABLED for now (build issues with MinGW)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/bullet/src/btBulletDynamicsCommon.h")
    set(HAS_BULLET TRUE)
    message(STATUS "Bullet found: ${CMAKE_CURRENT_SOURCE_DIR}/external/bullet")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS ON CACHE BOOL "" FORCE)
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)
    # Disable AUTOMOC for external Bullet library
    set(CMAKE_AUTOMOC OFF)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/bullet" "${CMAKE_CURRENT_BINARY_DIR}/bullet")
    set(CMAKE_AUTOMOC ON)
    add_compile_definitions(HAS_BULLET=1)
else()
    set(HAS_BULLET FALSE)
    message(STATUS "Bullet not found in external/bullet")
    add_compile_definitions(HAS_BULLET=0)
endif()

# ============================================================================
# VHACD (convex decomposition, from Bullet Extras)
# ============================================================================
if(HAS_BULLET AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/bullet/Extras/VHACD/public/VHACD.h")
    set(HAS_VHACD TRUE)
    message(STATUS "VHACD found in Bullet Extras")
    set(CMAKE_AUTOMOC OFF)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/bullet/Extras/VHACD" "${CMAKE_CURRENT_BINARY_DIR}/vhacd")
    set(CMAKE_AUTOMOC ON)
    add_compile_definitions(HAS_VHACD=1)
else()
    set(HAS_VHACD FALSE)
    message(STATUS "VHACD not found")
    add_compile_definitions(HAS_VHACD=0)
endif()

# ============================================================================
# 7-Zip 24+ (archive handling)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP/7zip/Common/StdAfx.h")
    set(HAS_7ZIP TRUE)
    message(STATUS "7-Zip found: ${CMAKE_CURRENT_SOURCE_DIR}/external/7zip")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/7zip" "${CMAKE_CURRENT_BINARY_DIR}/7zip")
    add_compile_definitions(HAS_7ZIP=1)
else()
    set(HAS_7ZIP FALSE)
    message(STATUS "7-Zip not found in external/7zip")
    add_compile_definitions(HAS_7ZIP=0)
endif()

# ============================================================================
# libigl 2.5+ (header-only geometry processing, depends on Eigen)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/libigl/include/igl/boundary_loop.h")
    set(HAS_LIBIGL TRUE)
    message(STATUS "libigl found: ${CMAKE_CURRENT_SOURCE_DIR}/external/libigl")
    add_compile_definitions(HAS_LIBIGL=1)
else()
    set(HAS_LIBIGL FALSE)
    message(STATUS "libigl not found in external/libigl")
    add_compile_definitions(HAS_LIBIGL=0)
endif()

# ============================================================================
# CGAL 6.1+ (computational geometry, via vcpkg)
# ============================================================================
find_package(CGAL QUIET)
if(CGAL_FOUND)
    set(HAS_CGAL TRUE)
    message(STATUS "CGAL found: ${CGAL_VERSION}")
    add_compile_definitions(HAS_CGAL=1)
else()
    set(HAS_CGAL FALSE)
    message(STATUS "CGAL not found (optional)")
    add_compile_definitions(HAS_CGAL=0)
endif()

# ============================================================================
# OpenVDB 12+ (volumetric, via vcpkg)
# ============================================================================
find_package(OpenVDB QUIET)
if(OpenVDB_FOUND)
    set(HAS_OPENVDB TRUE)
    message(STATUS "OpenVDB found")
    add_compile_definitions(HAS_OPENVDB=1)
else()
    set(HAS_OPENVDB FALSE)
    message(STATUS "OpenVDB not found (optional)")
    add_compile_definitions(HAS_OPENVDB=0)
endif()

# ============================================================================
# libuv (UV unwrapping, via vcpkg)
# ============================================================================
find_package(libuv QUIET)
if(libuv_FOUND)
    set(HAS_LIBUV TRUE)
    message(STATUS "libuv found")
    add_compile_definitions(HAS_LIBUV=1)
else()
    set(HAS_LIBUV FALSE)
    message(STATUS "libuv not found (optional)")
    add_compile_definitions(HAS_LIBUV=0)
endif()

# ============================================================================
# OpenSubdiv (subdivision surfaces, via vcpkg)
# ============================================================================
find_package(OpenSubdiv QUIET)
if(OpenSubdiv_FOUND)
    set(HAS_OPENSUBDIV TRUE)
    message(STATUS "OpenSubdiv found")
    add_compile_definitions(HAS_OPENSUBDIV=1)
else()
    set(HAS_OPENSUBDIV FALSE)
    message(STATUS "OpenSubdiv not found (optional)")
    add_compile_definitions(HAS_OPENSUBDIV=0)
endif()

# ============================================================================
# FMOD SDK removed — kseditor uses its own software audio engine
# ============================================================================

# ============================================================================
# Lua (optional scripting)
# ============================================================================
find_package(Lua QUIET)
if(Lua_FOUND)
    set(HAS_LUA TRUE)
    message(STATUS "Lua found: ${LUA_VERSION}")
    add_compile_definitions(HAS_LUA=1)
else()
    set(HAS_LUA FALSE)
    message(STATUS "Lua not found")
    add_compile_definitions(HAS_LUA=0)
endif()

# ============================================================================
# Python (optional scripting)
# ============================================================================
find_package(Python3 COMPONENTS Development QUIET)
if(Python3_FOUND)
    set(HAS_PYTHON TRUE)
    message(STATUS "Python found: ${Python3_VERSION}")
    add_compile_definitions(HAS_PYTHON=1)
else()
    set(HAS_PYTHON FALSE)
    message(STATUS "Python not found")
    add_compile_definitions(HAS_PYTHON=0)
endif()

if(Vulkan_FOUND)
    message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIRS}")
    add_compile_definitions(HAS_VULKAN=1)
else()
    message(STATUS "Vulkan SDK not found")
    add_compile_definitions(HAS_VULKAN=0)
endif()

file(GLOB_RECURSE ALL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/resources/*.cpp)

# plugin_stubs.cpp removed — no longer needed (real implementations compiled into kseditor_lib)

# Exclude plugin sources from main exe (they are compiled into the plugin DLL)
file(GLOB_RECURSE PLUGIN_SOURCES_EXCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/simulators/kunos/assettocorsa/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/simulators/kunos/assettocorsa/*.h
)
list(REMOVE_ITEM ALL_SOURCES ${PLUGIN_SOURCES_EXCLUDE})

# VR module re-enabled: OpenXR headers now compatible with MSVC 2026

# Re-add plugin sources that were excluded by PLUGIN_SOURCES_EXCLUDE but needed in the library
list(APPEND ALL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/simulators/kunos/assettocorsa/KN5Parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/simulators/kunos/assettocorsa/acFiles/FBXExporter.cpp
)

set(SOURCES ${ALL_SOURCES})

file(GLOB_RECURSE ALL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/*.h
)

list(REMOVE_ITEM ALL_HEADERS ${PLUGIN_SOURCES_EXCLUDE})

# VR module headers re-enabled

file(GLOB BROKEN_HEADERS
)

list(REMOVE_ITEM ALL_HEADERS ${BROKEN_HEADERS})
set(HEADERS ${ALL_HEADERS})

# Auto-reconfigure when source files change (mitigates GLOB_RECURSE staleness)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

add_library(kseditor_lib STATIC
    ${SOURCES} ${HEADERS}
)

if(MSVC)
    set_target_properties(kseditor_lib PROPERTIES
        COMPILE_PDB_NAME "kseditor_lib"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>"
    )
endif()

target_include_directories(kseditor_lib PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/core/Scripting
    ${PROJECT_SOURCE_DIR}/src/modules
    ${PROJECT_SOURCE_DIR}/src/plugins
    ${PROJECT_SOURCE_DIR}/src/plugins/simulators/kunos
    ${PROJECT_SOURCE_DIR}/resources/ui
    ${PROJECT_SOURCE_DIR}/src/core/3dprint
    ${PROJECT_SOURCE_DIR}/src/core/eventEditor
    ${PROJECT_SOURCE_DIR}/src/core/ServerConfigEditor
    "${Qt6Core_DIR}/../../../include/QtCore/${Qt6Core_VERSION}"
    "${Qt6Core_DIR}/../../../include/QtCore/${Qt6Core_VERSION}/QtCore/private"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows/include")
    target_include_directories(kseditor_lib PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows/include"
    )
endif()

if(HAS_EIGEN)
    target_include_directories(kseditor_lib PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/eigen"
    )
endif()

target_include_directories(kseditor_lib PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/openxr/include"
)

if(HAS_LIBIGL)
    target_include_directories(kseditor_lib PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/libigl/include"
    )
endif()

if(HAS_LIBUV)
    target_include_directories(kseditor_lib PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows/include"
    )
endif()

if(HAS_OPENSUBDIV)
    target_include_directories(kseditor_lib PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows/include"
    )
endif()

if(HAS_VHACD)
    target_include_directories(kseditor_lib PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/bullet/Extras/VHACD/public"
    )
endif()

target_link_libraries(kseditor_lib PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Quick3D Qt6::QuickControls2 Qt6::QuickWidgets Qt6::Widgets Qt6::Network Qt6::Multimedia Qt6::OpenGL Qt6::Sql Qt6::Charts Qt6::WebSockets
)

if(WIN32)
    target_link_libraries(kseditor_lib PRIVATE opengl32 dwmapi dwrite ole32 sapi)
endif()

# FMOD removed — using kseditor software audio engine

if(HAS_MIKKTSPACE)
    target_link_libraries(kseditor_lib PRIVATE mikktspace)
endif()

if(Vulkan_FOUND)
    target_include_directories(kseditor_lib PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(kseditor_lib PRIVATE ${Vulkan_LIBRARIES})
endif()

if(HAS_BULLET)
    target_link_libraries(kseditor_lib PRIVATE
        BulletDynamics
        BulletCollision
        LinearMath
    )
endif()

if(HAS_7ZIP)
    # 7zip symbols are duplicated when both kseditor.exe and ksAssettoCorsa.dll
    # consume kseditor_lib (e.g. IID_* from DllExports.obj vs SevenZipLibrary.obj).
    # The /FORCE:MULTIPLE flags on the exe and DLL handle this.
    target_link_libraries(kseditor_lib PRIVATE 7zip)
else()
    # Archive source files still need 7zip headers for compilation (unit tests etc.)
    target_include_directories(kseditor_lib PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/C"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP/Common"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP/Windows"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP/7zip"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP/7zip/Common"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP/7zip/Compress"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP/7zip/Crypto"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/7zip/CPP/7zip/Archive"
    )
endif()

if(Qt63DCore_FOUND)
    target_link_libraries(kseditor_lib PRIVATE Qt6::3DCore Qt6::3DRender Qt6::3DExtras Qt6::3DLogic Qt6::3DInput Qt6::3DAnimation)
    target_include_directories(kseditor_lib PRIVATE ${Qt6_INCLUDE_DIRS})
endif()

if(HAS_CGAL)
    target_link_libraries(kseditor_lib PRIVATE CGAL::CGAL)
endif()

if(HAS_OPENVDB)
    target_link_libraries(kseditor_lib PRIVATE OpenVDB::openvdb)
endif()

if(HAS_LIBUV)
    target_link_libraries(kseditor_lib PRIVATE libuv::uv)
endif()

if(HAS_OPENSUBDIV)
    target_link_libraries(kseditor_lib PRIVATE OpenSubdiv::osdCPU_static)
endif()

if(HAS_VHACD)
    target_link_libraries(kseditor_lib PRIVATE VHACD)
endif()

if(HAS_LUA)
    target_link_libraries(kseditor_lib PRIVATE Lua::Lua)
endif()

if(HAS_PYTHON)
    target_link_libraries(kseditor_lib PRIVATE Python3::Python)
endif()

if(MSVC)
    target_compile_options(kseditor_lib PRIVATE /Zc:__cplusplus /FS)
else()
    target_compile_options(kseditor_lib PRIVATE -fpermissive -Wa,-mbig-obj)
endif()

target_compile_definitions(kseditor_lib PRIVATE NOMINMAX)
target_compile_definitions(kseditor_lib PRIVATE HAS_3DPRINT=1)

add_executable(kseditor WIN32
    resources.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app_icon.rc
)

target_link_libraries(kseditor PRIVATE
    kseditor_lib
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Quick3D Qt6::QuickControls2 Qt6::QuickWidgets Qt6::Widgets Qt6::Network Qt6::Multimedia Qt6::OpenGL Qt6::Sql Qt6::Charts Qt6::WebSockets
)

target_include_directories(kseditor PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/modules
    ${PROJECT_SOURCE_DIR}/src/plugins
    ${PROJECT_SOURCE_DIR}/src/plugins/simulators/kunos
    ${PROJECT_SOURCE_DIR}/resources/ui
)

if(HAS_MIKKTSPACE)
    target_link_libraries(kseditor PRIVATE mikktspace)
endif()

if(HAS_BULLET)
    target_link_libraries(kseditor PRIVATE BulletDynamics BulletCollision LinearMath)
endif()

if(HAS_LIBUV)
    target_link_libraries(kseditor PRIVATE libuv::uv)
endif()

if(HAS_OPENSUBDIV)
    target_link_libraries(kseditor PRIVATE OpenSubdiv::osdCPU_static)
endif()

if(HAS_VHACD)
    target_link_libraries(kseditor PRIVATE VHACD)
endif()

if(Qt63DCore_FOUND)
    target_link_libraries(kseditor PRIVATE Qt6::3DCore Qt6::3DRender Qt6::3DExtras Qt6::3DLogic Qt6::3DInput Qt6::3DAnimation)
    target_include_directories(kseditor PRIVATE ${Qt6_INCLUDE_DIRS})
    add_definitions(-DHAS_QT3D=1)
else()
    add_definitions(-DHAS_QT3D=0)
endif()

set_target_properties(kseditor PROPERTIES
    OUTPUT_NAME "ksEditor"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/lib"
    COMPILE_PDB_NAME "ksEditor"
    COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>"
)

if(WIN32)
    add_custom_command(TARGET kseditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/bin/lib"
        COMMAND powershell -Command "Get-ChildItem '${CMAKE_CURRENT_SOURCE_DIR}/bin' -Filter '*.dll' | Move-Item -Destination '${CMAKE_CURRENT_SOURCE_DIR}/bin/lib/' -Force -ErrorAction SilentlyContinue"
        COMMENT "Moving DLLs to bin/lib/"
    )

    # FMOD removed — no DLL copy needed
endif()

if(Vulkan_FOUND)
    target_include_directories(kseditor PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(kseditor PRIVATE ${Vulkan_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(kseditor PRIVATE dwmapi opengl32 dwrite)
    target_compile_definitions(kseditor PRIVATE QT_DISABLE_ENTRYPOINT_FIX)
endif()

if(WIN32)
    set_target_properties(kseditor PROPERTIES
        ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
    )
endif()

set_property(TARGET kseditor PROPERTY CXX_STANDARD 17)
set_property(TARGET kseditor PROPERTY CXX_STANDARD_REQUIRED ON)

# /FORCE:MULTIPLE is required because 7zip static library IID_* symbols are
# duplicated when both kseditor.exe and ksAssettoCorsa.dll link kseditor_lib
# which contains both 7zip static archives and kseditor's own SevenZipLibrary.obj.
# Similar duplicates exist for Ribbon/RenderGraph/ModManager classes whose
# implementations appear in more than one translation unit within kseditor_lib.
if(MSVC)
    target_link_options(kseditor PRIVATE "LINKER:/FORCE:MULTIPLE")
endif()

# ============================================================================
# ksAssettoCorsa Plugin DLL
# ============================================================================

set(ASSETTOCORSA_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/simulators/kunos/assettocorsa)

set(ASSETTOCORSA_PLUGIN_MOC_HEADERS
    ${ASSETTOCORSA_PLUGIN_DIR}/QmlBridges.h
    ${ASSETTOCORSA_PLUGIN_DIR}/WorkshopModule.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KsContentPaths.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KsRunner.h
    ${ASSETTOCORSA_PLUGIN_DIR}/SetupComparison.h
    ${ASSETTOCORSA_PLUGIN_DIR}/acFiles/Kn5Previewer.h
    ${ASSETTOCORSA_PLUGIN_DIR}/acCSP/CspConfigParser.h
    ${ASSETTOCORSA_PLUGIN_DIR}/acCSP/CspConfigHandler.h
    ${ASSETTOCORSA_PLUGIN_DIR}/ACSharedMemory.h
    ${ASSETTOCORSA_PLUGIN_DIR}/ACLivePreviewBridge.h
    ${ASSETTOCORSA_PLUGIN_DIR}/AssetsLibraryModule.h
)

qt6_wrap_cpp(ASSETTOCORSA_MOC_SOURCES ${ASSETTOCORSA_PLUGIN_MOC_HEADERS})

set(ASSETTOCORSA_SOURCES
    ${ASSETTOCORSA_PLUGIN_DIR}/ksAssettoCorsa.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/AssetsLibraryModule.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/ContentBrowser.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/KsContentPaths.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/KsRunner.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/QmlBridges.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/SetupComparison.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/WorkshopModule.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/acCSP/CspConfigParser.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/acCSP/CspConfigHandler.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/acFiles/FBXExporter.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/acFiles/Kn5Previewer.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/acFiles/LODExporter.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/KSAnimFormat.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/KNHFormat.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/shaders/KsShaders.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/ACSharedMemory.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/ACDParser.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/KN5Decrypt.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/KN5Parser.cpp
    ${ASSETTOCORSA_PLUGIN_DIR}/ACLivePreviewBridge.cpp
    ${ASSETTOCORSA_MOC_SOURCES}
)

set(ASSETTOCORSA_HEADERS
    ${ASSETTOCORSA_PLUGIN_DIR}/assettocorsa.h
    ${ASSETTOCORSA_PLUGIN_DIR}/AssetsLibraryModule.h
    ${ASSETTOCORSA_PLUGIN_DIR}/ksAssettoCorsa_export.h
    ${ASSETTOCORSA_PLUGIN_DIR}/ContentBrowser.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KsContentPaths.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KsRunner.h
    ${ASSETTOCORSA_PLUGIN_DIR}/QmlBridges.h
    ${ASSETTOCORSA_PLUGIN_DIR}/SetupComparison.h
    ${ASSETTOCORSA_PLUGIN_DIR}/WorkshopModule.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KsIni.h
    ${ASSETTOCORSA_PLUGIN_DIR}/ksconfig.h
    ${ASSETTOCORSA_PLUGIN_DIR}/kssetup.h
    ${ASSETTOCORSA_PLUGIN_DIR}/acCSP/CspConfigParser.h
    ${ASSETTOCORSA_PLUGIN_DIR}/acCSP/CspConfigHandler.h
    ${ASSETTOCORSA_PLUGIN_DIR}/acFiles/FBXExporter.h
    ${ASSETTOCORSA_PLUGIN_DIR}/acFiles/Kn5Previewer.h
    ${ASSETTOCORSA_PLUGIN_DIR}/acFiles/LODExporter.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KSAnimFormat.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KNHFormat.h
    ${ASSETTOCORSA_PLUGIN_DIR}/shaders/KsShaders.h
    ${ASSETTOCORSA_PLUGIN_DIR}/ACSharedMemory.h
    ${ASSETTOCORSA_PLUGIN_DIR}/ACDParser.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KN5Decrypt.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KN5Types.h
    ${ASSETTOCORSA_PLUGIN_DIR}/KN5Parser.h
    ${ASSETTOCORSA_PLUGIN_DIR}/ACLivePreviewBridge.h
)

add_library(ksAssettoCorsa SHARED
    ${ASSETTOCORSA_SOURCES}
    ${ASSETTOCORSA_HEADERS}
)

set_target_properties(ksAssettoCorsa PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

target_compile_definitions(ksAssettoCorsa PRIVATE KSASSETTOCORSA_EXPORTS)

target_include_directories(ksAssettoCorsa PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/simulators/kunos
    ${ASSETTOCORSA_PLUGIN_DIR}
    ${ASSETTOCORSA_PLUGIN_DIR}/acCSP
    ${ASSETTOCORSA_PLUGIN_DIR}/acFiles
    ${ASSETTOCORSA_PLUGIN_DIR}/shaders
    "${Qt6Core_DIR}/../../../include/QtCore/${Qt6Core_VERSION}"
)

target_link_libraries(ksAssettoCorsa PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::Sql
    Qt6::OpenGL
    Qt6::Multimedia
    opengl32
    dwmapi
    dwrite
)

if(HAS_MIKKTSPACE)
    target_link_libraries(ksAssettoCorsa PRIVATE mikktspace)
endif()

if(Vulkan_FOUND)
    target_link_libraries(ksAssettoCorsa PRIVATE ${Vulkan_LIBRARIES})
    target_include_directories(ksAssettoCorsa PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()

set_target_properties(ksAssettoCorsa PROPERTIES
    OUTPUT_NAME "ksAssettoCorsa"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins"
)

target_link_libraries(ksAssettoCorsa PRIVATE kseditor_lib)

if(HAS_7ZIP)
    target_link_libraries(ksAssettoCorsa PRIVATE 7zip)
endif()

# /FORCE:MULTIPLE resolves duplicate Q_OBJECT symbols when plugin DLL links
# against kseditor_lib (static lib) which also contains the same Q_OBJECT classes.
target_link_options(ksAssettoCorsa PRIVATE /FORCE:MULTIPLE)

if(MSVC)
    target_compile_options(ksAssettoCorsa PRIVATE /Zc:__cplusplus /FS)
else()
    target_compile_options(ksAssettoCorsa PRIVATE -fpermissive -Wa,-mbig-obj)
endif()

target_compile_definitions(ksAssettoCorsa PRIVATE NOMINMAX)

add_dependencies(kseditor ksAssettoCorsa)

# ============================================================================
# Installation and Packaging
# ============================================================================

# Install main executable
install(TARGETS kseditor
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
)

# Install plugin
install(TARGETS ksAssettoCorsa
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
)

# Install resources (QML, shaders, icons, etc.)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources
    DESTINATION bin/resources
    FILES_MATCHING PATTERN "*.qml" PATTERN "*.vert" PATTERN "*.frag" PATTERN "*.svg" PATTERN "*.png" PATTERN "*.qss"
)

# Install Qt plugins via windeployqt (handled by post-build step)

# CPack/NSIS configuration for Windows installer
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "ksEditor")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional modding toolkit for Assetto Corsa")
    set(CPACK_PACKAGE_VENDOR "ksEditor Team")
    set(CPACK_PACKAGE_CONTACT "kseditor@github.com")
    set(CPACK_NSIS_COMPRESSOR "/SOLID zlib")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_HELP_LINK "https://github.com/kseditor/kseditor")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/kseditor/kseditor")
    set(CPACK_NSIS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin/ksEditor.exe")
    set(CPACK_NSIS_MENU_LINKS
        "https://github.com/kseditor/kseditor" "ksEditor GitHub"
        "https://github.com/kseditor/kseditor/issues" "Report Issue"
    )
    set(CPACK_PACKAGE_EXECUTABLES "ksEditor" "ksEditor")
    set(CPACK_CREATE_DESKTOP_LINKS "ksEditor")
    
    # Include license
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    
    # Package file name
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Windows-x64")
    
    include(CPack)
endif()
