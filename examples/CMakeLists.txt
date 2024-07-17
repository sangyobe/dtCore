include(DebugUtil)
include(GetDirectoryName)

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