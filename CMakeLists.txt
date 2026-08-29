cmake_minimum_required(VERSION 3.16)

project(kseditor
	VERSION 2.1.0
	LANGUAGES C CXX
)

cmake_policy(VERSION 3.16)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add MSVC specific flags required by Qt6
if(MSVC)
	add_compile_options(/permissive-)
	# Prevent Windows.h from defining min/max macros
	add_compile_definitions(NOMINMAX)
	# Force synchronous PDB writes for parallel compilation
	add_compile_options(/FS)
	# Allow large object files
	add_compile_options(/bigobj)
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

find_package(Qt6 COMPONENTS Core Gui Widgets Network Qml Quick Quick3D QuickControls2 QuickWidgets Multimedia OpenGL Sql Charts WebSockets Svg DataVisualization QUIET)
find_package(Vulkan QUIET)

if(NOT Qt6Svg_FOUND)
    message(STATUS "Qt6 Svg not found, PaintVector SVG features will be disabled")
endif()
if(NOT Qt6DataVisualization_FOUND)
    message(STATUS "Qt6 DataVisualization not found, CfdWidget will be disabled")
endif()

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
# Bullet Physics 3.25+ - DISABLED for compatibility
# ============================================================================
set(HAS_BULLET FALSE)
message(STATUS "Bullet Physics disabled for compatibility")
add_compile_definitions(HAS_BULLET=0)

# ============================================================================
# libigl 2.5+ (header-only geometry processing)
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
# Optional Scripting Languages
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

# ============================================================================
# zlib (for FBX compressed data)
# ============================================================================
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
	set(HAS_ZLIB TRUE)
	message(STATUS "zlib found: ${ZLIB_INCLUDE_DIRS}")
	add_compile_definitions(HAS_ZLIB=1)
else()
	set(HAS_ZLIB FALSE)
	message(STATUS "zlib not found")
	add_compile_definitions(HAS_ZLIB=0)
endif()

if(Vulkan_FOUND)
	message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIRS}")
	add_compile_definitions(HAS_VULKAN=1)
else()
	message(STATUS "Vulkan SDK not found")
	add_compile_definitions(HAS_VULKAN=0)
endif()

# ============================================================================
# Main Library and Application
# ============================================================================

# Collect source files
file(GLOB_RECURSE KSEDITOR_CORE_SOURCES
	"src/core/*.h"
	"src/core/*.cpp"
)

file(GLOB_RECURSE KSEDITOR_MODULES_SOURCES
	"src/modules/*.h"
	"src/modules/*.cpp"
)

file(GLOB_RECURSE KSEDITOR_PLUGINS_SOURCES
	"src/plugins/*.h"
	"src/plugins/*.cpp"
)

file(GLOB_RECURSE KSEDITOR_RESOURCES_UI_SOURCES
	"resources/ui/*.h"
	"resources/ui/*.cpp"
)

file(GLOB KSEDITOR_MAIN_SOURCES
	"src/main.cpp"
	"src/MainWindow.cpp"
	"resources.qrc"
)

# Enforce: src/core/ui must stay in resources/ui (single source of truth)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/ui")
	file(GLOB _core_ui_headers "${CMAKE_CURRENT_SOURCE_DIR}/src/core/ui/*.h")
	foreach(_hdr IN LISTS _core_ui_headers)
		get_filename_component(_name "${_hdr}" NAME)
		if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/${_name}")
			message(FATAL_ERROR "src/core/ui/${_name} has no counterpart in resources/ui — src/core/ui must stay in resources/ui")
		endif()
	endforeach()
endif()

# Create main kseditor library
add_library(kseditor_lib STATIC
	${KSEDITOR_CORE_SOURCES}
	${KSEDITOR_MODULES_SOURCES}
	${KSEDITOR_PLUGINS_SOURCES}
	${KSEDITOR_RESOURCES_UI_SOURCES}
)

target_include_directories(kseditor_lib PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/plugins"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/eigen"
)

# Add Qt6 private headers (for qzipreader_p.h etc.)
if(QT6_PATH)
	target_include_directories(kseditor_lib PUBLIC
		"${QT6_PATH}/include/QtCore/6.11.1"
	)
endif()

target_link_libraries(kseditor_lib PUBLIC
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
	dwmapi
	opengl32
)

if(HAS_MIKKTSPACE)
	target_link_libraries(kseditor_lib PUBLIC mikktspace)
endif()

target_compile_definitions(kseditor_lib PUBLIC
	KSASSETTOCORSA_EXPORTS
	KSASSETTOCORSAEVO_EXPORTS
)

if(Qt6Svg_FOUND)
	target_link_libraries(kseditor_lib PUBLIC Qt6::Svg)
endif()

if(ZLIB_FOUND)
	target_link_libraries(kseditor_lib PUBLIC ZLIB::ZLIB)
endif()

if(HAS_BULLET)
	target_link_libraries(kseditor_lib PUBLIC Bullet3Common BulletDynamics BulletCollision LinearMath)
endif()

if(Vulkan_FOUND)
	target_include_directories(kseditor_lib PUBLIC ${Vulkan_INCLUDE_DIRS})
	target_link_libraries(kseditor_lib PUBLIC ${Vulkan_LIBRARIES})
endif()

if(HAS_LUA)
	target_link_libraries(kseditor_lib PUBLIC ${LUA_LIBRARIES})
	target_include_directories(kseditor_lib PUBLIC ${LUA_INCLUDE_DIR})
endif()

if(HAS_PYTHON)
	target_link_libraries(kseditor_lib PUBLIC ${Python3_LIBRARIES})
	target_include_directories(kseditor_lib PUBLIC ${Python3_INCLUDE_DIRS})
endif()

# Create main executable
add_executable(kseditor
	${KSEDITOR_MAIN_SOURCES}
)

target_link_libraries(kseditor PRIVATE
	kseditor_lib
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
)

target_include_directories(kseditor PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/plugins"
)

target_compile_definitions(kseditor PRIVATE
	KSASSETTOCORSA_EXPORTS
	KSASSETTOCORSAEVO_EXPORTS
)

# Set output directory
set_target_properties(kseditor PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "ksEditor v${PROJECT_VERSION} configured successfully")
