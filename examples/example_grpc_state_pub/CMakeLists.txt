GetCurrentDirectoryName(PrjName)

project(${PrjName})

find_package(spdlog REQUIRED CONFIG)

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME}
    ${SRCS}
)
target_link_libraries(
    ${PROJECT_NAME}
    dtproto_grpc
    dtcore_grpc
    spdlog::spdlog
)

# get_target_property(${PROJECT_NAME}_link_libraries ${PROJECT_NAME} LINK_LIBRARIES)
# message(STATUS "${PROJECT_NAME}_link_libraries: ${${PROJECT_NAME}_link_libraries}")