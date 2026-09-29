# Standalone / headless C API library (no Qt, no Vulkan required for step).
# Include from root or use:
#   cmake -DKSENGINE_BUILD_C_API=ON ..

option(KSENGINE_BUILD_C_API "Build ksengine_c shared/static library" ON)

if(NOT KSENGINE_BUILD_C_API)
  return()
endif()

set(KSENGINE_C_SOURCES
  ${CMAKE_SOURCE_DIR}/src/engine/api/ksengine_c.cpp
)

# Prefer existing VehicleSimulator if present; otherwise compile-only stubs
if(EXISTS "${CMAKE_SOURCE_DIR}/src/engine/physics/VehicleSimulator.cpp")
  list(APPEND KSENGINE_C_SOURCES
    ${CMAKE_SOURCE_DIR}/src/engine/physics/VehicleSimulator.cpp
  )
endif()

add_library(ksengine_c STATIC ${KSENGINE_C_SOURCES})
target_include_directories(ksengine_c PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/engine
)
target_compile_definitions(ksengine_c PUBLIC KSENGINE_QT_FREE=1)
target_compile_features(ksengine_c PUBLIC cxx_std_17)

if(MSVC)
  target_compile_definitions(ksengine_c PRIVATE NOMINMAX)
endif()

message(STATUS "ksengine_c (headless C API) target enabled")
