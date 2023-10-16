if(BUILD_dtProto)
  # This assumes that gRPC and all its dependencies are already installed
  # on this system, so they can be located by find_package().

  # Find Protobuf installation
  # Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
  set(protobuf_MODULE_COMPATIBLE TRUE)
  find_package(Protobuf CONFIG REQUIRED)
  message(STATUS "Using protobuf ${Protobuf_VERSION}")
  message(STATUS "    Protobuf_INCLUDE_DIRS ${Protobuf_INCLUDE_DIRS}")
  message(STATUS "    Protobuf_LIBRARY ${Protobuf_LIBRARY}")
  message(STATUS "    Protobuf_LIBRARY_DEBUG ${Protobuf_LIBRARY_DEBUG}")
  message(STATUS "    Protobuf_PROTOC_EXECUTABLE ${Protobuf_PROTOC_EXECUTABLE}")
  
  set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)

  if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
  else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
  endif()

  if(BUILD_dtProto_gRPC)
    # Find gRPC installation
    # Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
    find_package(gRPC CONFIG REQUIRED)
    message(STATUS "Using gRPC ${gRPC_VERSION}")
    message(STATUS "    gRPC_INCLUDE_DIRS ${gRPC_INCLUDE_DIRS}")
    message(STATUS "    gRPC_LIBRARY ${gRPC_LIBRARY}")
    message(STATUS "    gRPC_LIBRARY_DEBUG ${gRPC_LIBRARY_DEBUG}")

    set(_GRPC_GRPCPP gRPC::grpc++)
    set(_GRPC_REFLECTION gRPC::grpc++_reflection)
    if(CMAKE_CROSSCOMPILING)
        find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
    else()
        set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
    endif()
  endif()

  # Target Art Protocol
  set(PROTOBUF_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto")
  add_subdirectory(proto)
  set(PROTOBUF_GEN_DIR ${PROTOBUF_GEN_DIR} PARENT_SCOPE)
endif()

add_subdirectory(core)
