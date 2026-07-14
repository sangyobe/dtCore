cmake_minimum_required(VERSION 3.13)
project(example_mcap_filesink)

list(APPEND CMAKE_PREFIX_PATH "$ENV{ARTF_INSTALL_DIR}/lib/cmake")
find_package(artf CONFIG REQUIRED)

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    artf::dtcore
    artf::dtproto
)
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    CXX_STANDARD 17
)

file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis_mcap.py
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE
)