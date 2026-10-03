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

if(MSVC)
	add_compile_options(/permissive-)
	add_compile_definitions(NOMINMAX)
	add_compile_options(/FS)
	add_compile_options(/bigobj)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeLists_root_qtfree_hook.cmake")

if(NOT KS_QT_FREE_BUILD AND NOT QT6_PATH)
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows")
	list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/installed/x64-windows")
endif()

set(QML_IMPORT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/qml CACHE PATH "QML import paths")

if(NOT KS_QT_FREE_BUILD)
	find_package(Qt6 COMPONENTS Core Gui Widgets Network Qml Quick Quick3D QuickControls2 QuickWidgets Multimedia OpenGL Sql Charts WebSockets Svg DataVisualization QUIET)
endif()
find_package(Vulkan QUIET)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/eigen/Eigen/Core")
	add_compile_definitions(HAS_EIGEN=1)
else()
	add_compile_definitions(HAS_EIGEN=0)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace/mikktspace.c")
	add_library(mikktspace STATIC "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace/mikktspace.c")
	target_include_directories(mikktspace PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/mikktspace")
	set_target_properties(mikktspace PROPERTIES C_STANDARD 11 AUTOMOC OFF)
	add_compile_definitions(HAS_MIKKTSPACE=1)
else()
	add_compile_definitions(HAS_MIKKTSPACE=0)
endif()

find_package(Bullet QUIET)
if(BULLET_FOUND OR TARGET BulletDynamics)
	add_compile_definitions(HAS_BULLET=1)
else()
	add_compile_definitions(HAS_BULLET=0)
endif()

if(Vulkan_FOUND)
	add_compile_definitions(HAS_VULKAN=1)
else()
	add_compile_definitions(HAS_VULKAN=0)
endif()

if(KS_QT_FREE_BUILD)
	add_compile_definitions(KSASSETTOCORSA_EXPORTS)
	include_directories(
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/simulator"
	)
	if(Vulkan_FOUND)
		include_directories(${Vulkan_INCLUDE_DIRS})
	endif()
	add_subdirectory(src/engine)
	if(KSIMULATOR_QT_FREE)
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeLists_ksimulator_QtFree.cmake")
			include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeLists_ksimulator_QtFree.cmake")
		elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_ksimulator_QtFree.cmake")
			include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_ksimulator_QtFree.cmake")
		endif()
	endif()
	enable_testing()
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/qtfree")
		add_subdirectory(tests/qtfree)
	endif()
	return()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/engine/Network/ksnet/CMakeLists.txt")
	add_subdirectory(src/core/engine/Network/ksnet EXCLUDE_FROM_ALL)
	add_compile_definitions(HAS_KSNET=1)
else()
	add_compile_definitions(HAS_KSNET=0)
endif()

add_subdirectory(src/engine)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/MinimalSimulator")
	add_subdirectory(examples/MinimalSimulator EXCLUDE_FROM_ALL)
endif()

file(GLOB_RECURSE KSEDITOR_CORE_SOURCES "src/core/*.h" "src/core/*.cpp" "src/core/**/*.h" "src/core/**/*.cpp")
list(REMOVE_ITEM KSEDITOR_CORE_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core/physics/weather/WeatherEditor.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core/physics/weather/WeatherEditor.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core/physics/weather/WeatherModule.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core/physics/weather/WeatherModule.cpp"
)
file(GLOB_RECURSE KSEDITOR_MODULES_SOURCES "src/modules/*.h" "src/modules/*.cpp")
file(GLOB_RECURSE KSEDITOR_PLUGINS_SOURCES "src/plugins/*.h" "src/plugins/*.cpp")
file(GLOB_RECURSE KSEDITOR_RESOURCES_UI_SOURCES "resources/ui/*.h" "resources/ui/*.cpp")
file(GLOB KSEDITOR_MAIN_SOURCES "src/main.cpp" "src/MainWindow.cpp" resources.qrc)

add_library(kslib STATIC
	${KSEDITOR_CORE_SOURCES}
	${KSEDITOR_MODULES_SOURCES}
	${KSEDITOR_PLUGINS_SOURCES}
	${KSEDITOR_RESOURCES_UI_SOURCES}
	"src/EditorApp.cpp"
)

target_include_directories(kslib PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core/engine"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/plugins"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/eigen"
)

target_link_libraries(kslib PUBLIC
	ksengine Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Quick
	Qt6::Quick3D Qt6::QuickControls2 Qt6::QuickWidgets Qt6::Multimedia Qt6::OpenGL
	Qt6::Sql Qt6::Charts Qt6::WebSockets dwmapi opengl32
)
if(TARGET mikktspace)
	target_link_libraries(kslib PUBLIC mikktspace)
endif()
target_compile_definitions(kslib PUBLIC KSASSETTOCORSA_EXPORTS KSASSETTOCORSAEVO_EXPORTS)
if(Vulkan_FOUND)
	target_include_directories(kslib PUBLIC ${Vulkan_INCLUDE_DIRS})
	target_link_libraries(kslib PUBLIC ${Vulkan_LIBRARIES})
endif()

add_executable(kseditor ${KSEDITOR_MAIN_SOURCES})
target_link_libraries(kseditor PRIVATE kslib Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(kseditor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/src/core")

# Simulator application - includes FeatureMethods + Telemetry
add_executable(SimulatorApp
	"src/SimulatorApp.cpp"
	"src/simulator/InputManager.cpp"
	"src/simulator/CameraController.cpp"
	"src/simulator/DrivingRenderer.cpp"
	"src/simulator/SimulationLoop.cpp"
	"src/simulator/SimulationLoop_FeatureMethods.cpp"
	"src/simulator/SimulationLoop_Telemetry.cpp"
	"src/simulator/SimulatorAudio.cpp"
	"src/simulator/AIController.cpp"
	"src/simulator/RaceSessionManager.cpp"
	"src/simulator/DashboardOverlay.cpp"
	"src/simulator/SetupGarage.cpp"
	"src/simulator/TelemetryOverlay.cpp"
	"src/simulator/TrackLoader.cpp"
	"src/simulator/MultiCarManager.cpp"
	"src/simulator/PostProcessing.cpp"
	"src/simulator/ReplayRecorder.cpp"
	"src/simulator/GameMenuOverlay.cpp"
)

set_target_properties(SimulatorApp PROPERTIES AUTOMOC ON)
target_link_libraries(SimulatorApp PRIVATE kslib Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL opengl32)
target_include_directories(SimulatorApp PRIVATE
	"src/simulator" "src/core/engine/Network" "src/core/engine/Audio"
	"src/core/engine/Graphics" "src/core/engine/Physics" "src/core/engine/Devices"
	"${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/src/core"
)
set_target_properties(SimulatorApp PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_target_properties(kseditor PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

message(STATUS "ksEditor v${PROJECT_VERSION} configured - Driving Simulator Engine")
