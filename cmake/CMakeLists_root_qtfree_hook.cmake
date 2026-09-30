# Root hook: Qt-free build switch for ksengine + SimulatorApp.
#
#   cmake -DKSENGINE_QT_FREE=ON   -> ksengine only, no Qt anywhere
#   cmake -DKSIMULATOR_QT_FREE=ON -> ksengine + Qt-free SimulatorApp
#
# Included from the root CMakeLists.txt right after the compiler flags, so the
# switch is known before any find_package(Qt6)/AUTOMOC decision is taken.

option(KSENGINE_QT_FREE "Build ksengine without Qt" OFF)
option(KSIMULATOR_QT_FREE "Build SimulatorApp without Qt" OFF)

if(KSENGINE_QT_FREE OR KSIMULATOR_QT_FREE)
	set(KS_QT_FREE_BUILD ON)
else()
	set(KS_QT_FREE_BUILD OFF)
endif()

if(KS_QT_FREE_BUILD)
	# Nothing in this build may pull Qt in, neither by link nor by moc/uic/rcc.
	set(CMAKE_AUTOMOC OFF)
	set(CMAKE_AUTORCC OFF)
	set(CMAKE_AUTOUIC OFF)
	message(STATUS "Qt-free build: KSENGINE_QT_FREE=${KSENGINE_QT_FREE} KSIMULATOR_QT_FREE=${KSIMULATOR_QT_FREE}")
endif()
