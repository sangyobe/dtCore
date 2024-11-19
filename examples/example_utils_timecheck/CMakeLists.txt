cmake_minimum_required(VERSION 3.13)
project(example_utils_timecheck)

list(APPEND CMAKE_PREFIX_PATH "$ENV{ARTF_INSTALL_DIR}/lib/cmake")
find_package(artf CONFIG REQUIRED)

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    artf::dtcore
)
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    CXX_STANDARD 17
)