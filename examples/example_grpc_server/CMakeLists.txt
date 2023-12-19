GetCurrentDirectoryName(PrjName)

project(${PrjName})

# find_package(spdlog REQUIRED CONFIG)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_REFLECTION gRPC::grpc++_reflection)

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME}
    ${SRCS}
)
target_link_libraries(
    ${PROJECT_NAME}
    dtproto_grpc
#    spdlog::spdlog
    ${_PROTOBUF_LIBPROTOBUF}
    ${_GRPC_GRPCPP}
    ${_GRPC_REFLECTION}
)
# target_link_options(
#     ${PROJECT_NAME}
#     PRIVATE -Wl,--whole-archive
# )

get_target_property(${PROJECT_NAME}_link_libraries ${PROJECT_NAME} LINK_LIBRARIES)
message(STATUS "${PROJECT_NAME}_link_libraries: ${${PROJECT_NAME}_link_libraries}")