CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(ImageGraphCut)
ENABLE_LANGUAGE(CXX)

FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})

SET(CMAKE_MODULE_PATH ${ImageGraphCut_SOURCE_DIR}/CMake)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
FIND_PACKAGE(Metis REQUIRED)

SET(IMAGECUT_SRCS
  src/ImageGraphCut.cxx
  src/ImageToGraphFilter.h
  src/METISTools.cxx
  src/METISTools.h)

ADD_LIBRARY(image_graph_cut_internal ${IMAGECUT_SRCS})
ADD_EXECUTABLE(image_graph_cut src/ImageGraphCutMain.cxx)

# Configure METIS 
INCLUDE_DIRECTORIES(${METIS_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(image_graph_cut_internal ${METIS_LIBRARIES} ${ITK_LIBRARIES})
TARGET_LINK_LIBRARIES(image_graph_cut image_graph_cut_internal)

# Configure Python bindings
SET(BUILD_PYTHON OFF CACHE BOOL "Build Python bindings")
IF(BUILD_PYTHON)
  SET(PYBIND11_FINDPYTHON ON)
  ADD_SUBDIRECTORY(external/pybind11)
  INCLUDE_DIRECTORIES(external/pybind11)
  pybind11_add_module(picsl_image_graph_cut src/bindings/ImageGraphCutPythonBindings.cxx)
  TARGET_INCLUDE_DIRECTORIES(picsl_image_graph_cut PRIVATE src)
  TARGET_LINK_LIBRARIES(picsl_image_graph_cut PUBLIC image_graph_cut_internal)
  install(TARGETS picsl_image_graph_cut DESTINATION .)

ENDIF()
