# Example targets
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