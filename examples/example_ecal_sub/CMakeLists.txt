cmake_minimum_required(VERSION 3.13)
project(example_ecal_sub)

list(APPEND CMAKE_PREFIX_PATH "$ENV{ARTF_INSTALL_DIR}/lib/cmake")
find_package(artf CONFIG REQUIRED)
find_package(eCAL REQUIRED)

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    artf::dtproto
    eCAL::core
)
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    CXX_STANDARD 17
)