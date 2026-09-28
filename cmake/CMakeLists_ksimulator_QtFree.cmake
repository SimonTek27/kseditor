# Qt-free simulator executable — sources under src/simulator + engine native render
if(NOT KSIMULATOR_QT_FREE AND NOT KSENGINE_QT_FREE)
  return()
endif()

find_package(Vulkan REQUIRED)

set(KSIM_SOURCES
  ${CMAKE_SOURCE_DIR}/src/simulator/SimulatorApp.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/SimulationLoop.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/InputManager.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/CameraController.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/DashboardOverlay.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/GameMenuOverlay.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/NetworkManager.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/SetupGarage.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/TelemetryOverlay.cpp
  ${CMAKE_SOURCE_DIR}/src/simulator/MultiCarManager.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/sim/UiRenderer.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/sim/GpuProfiler.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/devices/DeviceManager.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/devices/DirectInputJoystick.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/devices/xinput/XInputDevice.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/devices/FFBBridge.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/devices/simracing/FFBSDKFactory.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/physics/VehicleSimulator.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/physics/PacejkaTireModel.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/physics/AeroModel.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/physics/EngineModel.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/physics/DifferentialModel.cpp
  ${CMAKE_SOURCE_DIR}/src/engine/physics/SuspensionModel.cpp
)

if(EXISTS "${CMAKE_SOURCE_DIR}/src/engine/sim/NativeRenderer.cpp")
  list(APPEND KSIM_SOURCES ${CMAKE_SOURCE_DIR}/src/engine/sim/NativeRenderer.cpp)
endif()

set(_ksim_existing "")
foreach(f ${KSIM_SOURCES})
  if(EXISTS "${f}")
    list(APPEND _ksim_existing ${f})
  else()
    message(STATUS "ksimulator: skip missing ${f}")
  endif()
endforeach()

add_executable(ksimulator WIN32 ${_ksim_existing})
target_compile_definitions(ksimulator PRIVATE KSENGINE_QT_FREE=1 HAS_VEHICLE_SIM=1 HAS_FFB=1)
target_include_directories(ksimulator PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/engine
  ${CMAKE_SOURCE_DIR}/src/engine/physics
  ${CMAKE_SOURCE_DIR}/src/engine/devices
  ${CMAKE_SOURCE_DIR}/src/engine/sim
  ${CMAKE_SOURCE_DIR}/src/simulator
  ${Vulkan_INCLUDE_DIRS}
)
target_link_libraries(ksimulator PRIVATE ${Vulkan_LIBRARIES})
if(WIN32)
  target_link_libraries(ksimulator PRIVATE dinput8 dxguid xinput ole32 user32 gdi32)
endif()
message(STATUS "ksimulator: app=src/simulator, render=src/engine/sim")
