GetCurrentDirectoryName(PrjName)

project(${PrjName})

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME}
    ${SRCS}
)
target_link_libraries(
    ${PROJECT_NAME}
    dtproto
    spdlog::spdlog
)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    "${CMAKE_SOURCE_DIR}/extlib/mcap/cpp/mcap/include"
)
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    CXX_STANDARD 17
)