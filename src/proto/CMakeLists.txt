cmake_minimum_required(VERSION 3.12)

project(dtProto
  LANGUAGES C CXX
  HOMEPAGE_URL https://github.hmc.co.kr/art_framework/dtProto
  DESCRIPTION "HMC's Art Framework Network Protocol Library"
  VERSION ${CMAKE_PROJECT_VERSION}
)

# Get protobuf/grpc generated file output path
function (get_pb_out_path pb_proto pb_src_dir pb_bin_dir pb_out_path)
    get_filename_component(pb_proto_path "${pb_proto}" PATH)
    string(LENGTH ${pb_src_dir} pb_src_dir_len)
    string(SUBSTRING ${pb_proto_path} ${pb_src_dir_len} -1 pb_proto_relative_path)
    string(CONCAT pb_proto_out_path ${pb_bin_dir} ${pb_proto_relative_path})
    set("${pb_out_path}" "${pb_proto_out_path}" PARENT_SCOPE)
endfunction()

# ART Messages
set(pb_proto_srcs "")
set(pb_proto_hdrs "")
set(pb_grpc_srcs "")
set(pb_grpc_hdrs "")
set(pb_gen_dir ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE PB_SRCS "*.proto")
foreach(pb_src ${PB_SRCS})
    # message(STATUS "pb_src: ${pb_src}")
    get_filename_component(pb_proto_path "${pb_src}" PATH)
    get_filename_component(pb_proto_name "${pb_src}" NAME_WE)
    get_pb_out_path("${pb_src}" "${CMAKE_CURRENT_SOURCE_DIR}" "${pb_gen_dir}" pb_bin_path)
    string(CONCAT pb_proto_src ${pb_bin_path} "/" "${pb_proto_name}" ".pb.cc")
    string(CONCAT pb_proto_hdr ${pb_bin_path} "/" "${pb_proto_name}" ".pb.h")
    string(CONCAT pb_grpc_src ${pb_bin_path} "/" "${pb_proto_name}" ".grpc.pb.cc")
    string(CONCAT pb_grpc_hdr ${pb_bin_path} "/" "${pb_proto_name}" ".grpc.pb.h")
    # message(STATUS "pb_proto_src: ${pb_proto_src}")
    # message(STATUS "pb_proto_hdr: ${pb_proto_hdr}")
    # message(STATUS "pb_grpc_src: ${pb_grpc_src}")
    # message(STATUS "pb_grpc_hdr: ${pb_grpc_hdr}")

    if(BUILD_dtProto_gRPC)
      add_custom_command(
        OUTPUT "${pb_proto_src}" "${pb_proto_hdr}" "${pb_grpc_src}" "${pb_grpc_hdr}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
          --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
          -I "${CMAKE_CURRENT_SOURCE_DIR}"
          -I "${pb_proto_path}"
          --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
          "${pb_src}"
        DEPENDS
          "${pb_src}"
      )
    else()
      add_custom_command(
        OUTPUT "${pb_proto_src}" "${pb_proto_hdr}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
          -I "${CMAKE_CURRENT_SOURCE_DIR}"
          -I "${pb_proto_path}"
          "${pb_src}"
        DEPENDS
          "${pb_src}"
      )
    endif()
    
    list(APPEND pb_proto_srcs "${pb_proto_src}")
    list(APPEND pb_proto_hdrs "${pb_proto_hdr}")
    list(APPEND pb_grpc_srcs "${pb_grpc_src}")
    list(APPEND pb_grpc_hdrs "${pb_grpc_hdr}")
endforeach()

# message(STATUS "pb_proto_srcs: ${pb_proto_srcs}")
# message(STATUS "pb_proto_hdrs: ${pb_proto_hdrs}")
# message(STATUS "pb_grpc_srcs: ${pb_grpc_srcs}")
# message(STATUS "pb_grpc_hdrs: ${pb_grpc_hdrs}")

# Include generated *.pb.h files
set(PROTOBUF_GEN_DIR ${pb_gen_dir} PARENT_SCOPE)


#####################################################################
# generate version.h
set(version_h_in ${CMAKE_CURRENT_SOURCE_DIR}/dtProto/version.h.in)
set(version_h ${CMAKE_CURRENT_BINARY_DIR}/dtProto/version.h)
configure_file(${version_h_in} ${version_h} @ONLY)

#####################################################################
# dtProto library
#
add_library(dtproto STATIC
  ${pb_proto_srcs}
  ${pb_proto_hdrs}
)
target_include_directories(dtproto PUBLIC
  ${Protobuf_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(dtproto PUBLIC 
  ${_PROTOBUF_LIBPROTOBUF}
)

# generate pkg-config.pc
set(pc_target dtproto)
set(pc_name dtProto)
set(pc_desc "HMC Art Framework Network Protocol Library")
set(pc_url "${CMAKE_PROJECT_HOMEPAGE_URL}")
set(pc_version "${PROJECT_VERSION}")
configure_file(${CMAKE_SOURCE_DIR}/cmake/package.pc.in "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/pkgconfig/${pc_target}.pc" @ONLY)

# install
install(TARGETS dtproto 
        INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        RUNTIME  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
install(DIRECTORY            ${CMAKE_CURRENT_BINARY_DIR}/dtProto
        DESTINATION          ${CMAKE_INSTALL_PREFIX}/include
)
install(DIRECTORY            ${CMAKE_CURRENT_SOURCE_DIR}/dtProto
        DESTINATION          ${CMAKE_INSTALL_PREFIX}/include
)
install(FILES                "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/pkgconfig/${pc_target}.pc"
        DESTINATION          "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/"
)


#####################################################################
# dtProto_gRPC library
#
if(BUILD_dtProto_gRPC)
  add_library(dtproto_grpc STATIC
    ${pb_grpc_srcs}
    ${pb_grpc_hdrs}
    ${pb_proto_srcs}
    ${pb_proto_hdrs})
  target_include_directories(dtproto_grpc PUBLIC
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
  )
  target_link_libraries(dtproto_grpc PUBLIC 
    ${_PROTOBUF_LIBPROTOBUF}
    ${_GRPC_GRPCPP}
    ${_GRPC_REFLECTION}
  )
  
  # generate pkg-config.pc
  set(pc_target dtproto_grpc)
  set(pc_name dtProto gRPC service binding)
  set(pc_desc "HMC Art Framework Network Protocol Library (gRPC service included)")
  set(pc_url "https://www.hmc.co.kr/art_framework/dtproto")
  set(pc_version "1.0.0.0")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/package.pc.in "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/pkgconfig/${pc_target}.pc" @ONLY)

  # install
  install(TARGETS dtproto_grpc
          INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include
          RUNTIME  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
          LIBRARY  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
          ARCHIVE  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  )
  install(FILES                "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/pkgconfig/${pc_target}.pc"
          DESTINATION          "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/"
  )
endif()
