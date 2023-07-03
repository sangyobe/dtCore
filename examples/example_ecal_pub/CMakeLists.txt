GetCurrentDirectoryName(PrjName)

project(${PrjName})

find_package(eCAL REQUIRED)

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME}
    ${SRCS}
)
target_link_libraries(
    ${PROJECT_NAME}
    dtproto
    eCAL::core
)