cmake_minimum_required(VERSION 3.13)
project(example_rpc_service_server)

list(APPEND CMAKE_PREFIX_PATH "$ENV{ARTF_INSTALL_DIR}/lib/cmake")
find_package(artf CONFIG REQUIRED)
find_package(rlabproto CONFIG REQUIRED)

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    artf::dtcore_grpc
    # artf::dtproto_grpc
    rlabproto::rlabproto_static
)
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    CXX_STANDARD 17
)