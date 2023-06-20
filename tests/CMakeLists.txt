cmake_minimum_required(VERSION 3.13)

project(dtCoreTest C CXX)

cmake_policy(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${HOME}/.local/lib;${CMAKE_MODULE_PATH}")
# include(cmake/common.cmake)

# Eigen
SET(EIGEN3_ROOT_DIR "$ENV{EIGEN3_ROOT}")
IF(NOT EIGEN3_ROOT_DIR)
    MESSAGE(FATAL_ERROR "Please point the environment variable EIGEN3_ROOT to the include directory of your Eigen3 installation.")
ENDIF()
find_package(Eigen3 3.3.7 REQUIRED)
if(NOT Eigen3_FOUND)
    MESSAGE(FATAL_ERROR "Eigen3 not found.")
endif()
MESSAGE(STATUS "Eigen3 was found at ${EIGEN3_INCLUDE_DIR}")
INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})

# Test targets
foreach(_target examplePoly)
  add_executable(
    ${_target}
    "${_target}.cpp"
  )
  target_include_directories(
    ${_target}
    PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
  )
endforeach()