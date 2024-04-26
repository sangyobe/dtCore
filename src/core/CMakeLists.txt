set(CORE_SRCS "")
set(CORE_HDRS "")

if(BUILD_dtCore)
    file(GLOB_RECURSE CORE_SRCS "./thread/*.cpp" "./utils/*.cpp")
    message(STATUS "CORE_SRCS: ${CORE_SRCS}")

    if(CORE_SRCS)
        add_library(dtcore STATIC
            ${CORE_SRCS}
            ${CORE_HDRS})
        target_include_directories(dtcore PUBLIC
            ${CMAKE_SOURCE_DIR}/include
        )
        # target_link_libraries(dtcore PUBLIC 
        # )

        # generate pkg-config.pc
        set(pc_target dtcore)
        set(pc_name dtCore library)
        set(pc_desc "${CMAKE_PROJECT_DESCRIPTION}")
        set(pc_url "${CMAKE_PROJECT_HOMEPAGE_URL}") 
        set(pc_version "${CMAKE_PROJECT_VERSION}") 
        configure_file(${CMAKE_SOURCE_DIR}/cmake/package.pc.in "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/pkgconfig/${pc_target}.pc" @ONLY)

        # install
        install(TARGETS dtcore
                INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include
                RUNTIME  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
                LIBRARY  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
                ARCHIVE  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        )
        install(FILES                "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/pkgconfig/${pc_target}.pc"
                DESTINATION          "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/"
        )
    endif()
endif()

if(BUILD_dtProto AND BUILD_dtProto_gRPC AND BUILD_dtCore_gRPC)
    file(GLOB_RECURSE GRPC_SRCS "./daq/grpc/*.cpp")
    list(APPEND CORE_SRCS ${GRPC_SRCS})
    message(STATUS "GRPC_SRCS: ${GRPC_SRCS}")
    message(STATUS "CORE_SRCS: ${CORE_SRCS}")

    if(CORE_SRCS)
        add_library(dtcore_grpc STATIC
            ${CORE_SRCS}
            ${CORE_HDRS})
        target_include_directories(dtcore_grpc PUBLIC
            ${Protobuf_INCLUDE_DIRS}
            ${CMAKE_CURRENT_BINARY_DIR}
        )
        target_link_libraries(dtcore_grpc PUBLIC 
            ${_PROTOBUF_LIBPROTOBUF}
            ${_GRPC_GRPCPP}
            ${_GRPC_REFLECTION}
        )

        # generate pkg-config.pc
        set(pc_target dtcore_grpc)
        set(pc_name dtCore gRPC service binding)
        set(pc_desc "${CMAKE_PROJECT_DESCRIPTION}")
        set(pc_url "${CMAKE_PROJECT_HOMEPAGE_URL}") 
        set(pc_version "${CMAKE_PROJECT_VERSION}") 
        configure_file(${CMAKE_SOURCE_DIR}/cmake/package.pc.in "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/pkgconfig/${pc_target}.pc" @ONLY)

        # install
        install(TARGETS dtcore_grpc
                INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include
                RUNTIME  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
                LIBRARY  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
                ARCHIVE  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        )
        install(FILES                "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/pkgconfig/${pc_target}.pc"
                DESTINATION          "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/"
        )
    endif()

endif()