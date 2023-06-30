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

# This assumes that gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
  find_program(_PROTOBUF_PROTOC protoc)
else()
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Target dtCore modules
add_subdirectory(src)


# Unit tests
add_subdirectory(tests)
