cmake_minimum_required(VERSION 3.13)

project(dtCore C CXX)

cmake_policy(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

#add_compile_options( -g -Wall )

#set(CMAKE_C_FLAGS "-Wall -std=c99")
#set(CMAKE_C_FLAGS_DEBUG "-Wall -g")
#set(CMAKE_C_FLAGS_RELEASE "-Wall -O4 -DNDEBUG")
#set(CMAKE_CXX_FLAGS "-Wall -std=c++0x -stdlib=libc++")
#set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g")
#set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O4 -DNDEBUG")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")


# --------------------------------------------------------
# Build options
# use it that way cmake .. -DBUILD_EXAMPLES=ON -DBUILD_dtProto=ON
# --------------------------------------------------------
OPTION(BUILD_DOCS           "Build documents"                         OFF)
OPTION(BUILD_UNIT_TESTS     "Build unit test"                         OFF)
OPTION(BUILD_EXAMPLES       "Build examples"                          ON)
OPTION(BUILD_EXAMPLES_eCAL  "Build eCAL examples"                     OFF)
OPTION(BUILD_dtProto        "Build dtProto headers and library"       OFF)
OPTION(BUILD_dtProto_gRPC   "Build dtProto library with gRPC support" OFF)
OPTION(dtCore_USE_EIGEN3    "Use Eigen3 as the default math library"  OFF)


# --------------------------------------------------------
# Find & Include 3rd-party modules
# --------------------------------------------------------
# include(cmake/common.cmake)

if(dtCore_USE_EIGEN3)
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
endif()



# --------------------------------------------------------
# Build dtCore modules
# --------------------------------------------------------
add_subdirectory(src)


# --------------------------------------------------------
# Build unit tests
# --------------------------------------------------------
if(BUILD_UNIT_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()


# --------------------------------------------------------
# Build examples
# --------------------------------------------------------
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()


# --------------------------------------------------------
# Install
# --------------------------------------------------------
install(DIRECTORY ./include/dtCore
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)


# --------------------------------------------------------
# Finalize configuration and print options
# --------------------------------------------------------
message(STATUS "BUILD_DOCS                                     : ${BUILD_DOCS}")
message(STATUS "BUILD_UNIT_TESTS                               : ${BUILD_UNIT_TESTS}")
message(STATUS "BUILD_EXAMPLES                                 : ${BUILD_EXAMPLES}")
message(STATUS "BUILD_EXAMPLES_eCAL                            : ${BUILD_EXAMPLES_eCAL}")
message(STATUS "BUILD_dtProto                                  : ${BUILD_dtProto}")
message(STATUS "BUILD_dtProto_gRPC                             : ${BUILD_dtProto_gRPC}")
message(STATUS "dtCore_USE_EIGEN3                              : ${dtCore_USE_EIGEN3}")
