include(DebugUtil)
include(GetDirectoryName)

include_directories(
    "${CMAKE_SOURCE_DIR}/extlib/spdlog/include"
    "${CMAKE_SOURCE_DIR}/extlib/yaml-cpp/include"
    "${CMAKE_SOURCE_DIR}/extlib/mcap/cpp/mcap/include"
    "${CMAKE_SOURCE_DIR}/include"
)
link_directories(
    "${CMAKE_SOURCE_DIR}/lib"
)

if(BUILD_dtProto)
  include_directories("${PROTOBUF_GEN_DIR}")
endif()

# Mcap example targets
if(BUILD_dtProto AND BUILD_EXAMPLES_MCAP)
  file(GLOB EXAMPLE_DIRS "example_mcap_*")
  foreach(example_dir ${EXAMPLE_DIRS})
      add_subdirectory(${example_dir})
  endforeach()
endif()

# eCAL example targets
if(BUILD_dtProto AND BUILD_EXAMPLES_eCAL)
  file(GLOB EXAMPLE_DIRS "example_ecal_*")
  foreach(example_dir ${EXAMPLE_DIRS})
      add_subdirectory(${example_dir})
  endforeach()
endif()

# gRPC example targets
if(BUILD_dtProto AND BUILD_dtProto_gRPC AND BUILD_EXAMPLES_gRPC)
  file(GLOB EXAMPLE_DIRS "example_grpc_*")
  foreach(example_dir ${EXAMPLE_DIRS})
      add_subdirectory(${example_dir})
  endforeach()
endif()

# other examples
file(GLOB EXAMPLE_DIRS "example_thread_*" "example_log_*" "example_utils_*")

foreach(example_dir ${EXAMPLE_DIRS})
    add_subdirectory(${example_dir})
endforeach()
