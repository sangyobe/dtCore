include(DebugUtil)
include(GetDirectoryName)

# eCAL example targets
if(BUILD_dtProto AND BUILD_EXAMPLES_eCAL)
  file(GLOB EXAMPLE_DIRS "example_ecal_*")
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