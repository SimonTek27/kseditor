option(KSENGINE_QT_FREE "Build engine without Qt" OFF)
option(KSIMULATOR_QT_FREE "Build ksimulator without Qt" OFF)

if(KSENGINE_QT_FREE)
  add_subdirectory(src/engine)
endif()

if(KSIMULATOR_QT_FREE)
  include(${CMAKE_SOURCE_DIR}/cmake/CMakeLists_ksimulator_QtFree.cmake)
endif()
