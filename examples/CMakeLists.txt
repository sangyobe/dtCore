include(DebugUtil)
include(GetDirectoryName)

if(BUILD_dtProto)
  include_directories("${PROTOBUF_GEN_DIR}")
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

# dtTrajectory example targets
foreach(_target examplePoly exampleBezier exampleScurve)
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