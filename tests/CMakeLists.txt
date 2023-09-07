cmake_minimum_required(VERSION 3.12)
project("test_${CMAKE_PROJECT_NAME}")

cmake_policy(SET CMP0074 NEW)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.13.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


include_directories(
    "${CMAKE_SOURCE_DIR}/include"
)
link_directories(
    "${CMAKE_SOURCE_DIR}/lib"
)

#link_directories(
#    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#)

file(GLOB test_dtTrajectory_SRCS test_dttraj*.cpp test_dttraj*.h)
add_executable(test_dtTrajectory ${test_dtTrajectory_SRCS})
add_test(NAME test_dtTrajectory COMMAND test_dtTrajectory)
target_link_libraries(test_dtTrajectory gtest gtest_main pthread)
