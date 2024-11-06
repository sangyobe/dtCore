if(CMAKE_VERSION VERSION_LESS 3.5.0)
    message(FATAL_ERROR "This file relies on consumers using CMake 3.5.0 or greater.")
endif()

cmake_policy(PUSH)
cmake_policy(VERSION 3.5)

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Protect against multiple inclusion, which would fail when already imported targets are added once more.
set(_targetsDefined)
set(_targetsNotDefined)
set(_expectedTargets)

foreach(_expectedTarget artf::dtcore artf::dtcore_grpc artf::dtproto artf::dtproto_grpc)
    list(APPEND _expectedTargets ${_expectedTarget})

    if(NOT TARGET ${_expectedTarget})
        list(APPEND _targetsNotDefined ${_expectedTarget})
    endif()

    if(TARGET ${_expectedTarget})
        list(APPEND _targetsDefined ${_expectedTarget})
    endif()
endforeach()

if("${_targetsDefined}" STREQUAL "${_expectedTargets}")
    unset(_targetsDefined)
    unset(_targetsNotDefined)
    unset(_expectedTargets)
    set(CMAKE_IMPORT_FILE_VERSION)
    cmake_policy(POP)
    return()
endif()

if(NOT "${_targetsDefined}" STREQUAL "")
    message(FATAL_ERROR "Some (but not all) targets in this export set were already defined.\nTargets Defined: ${_targetsDefined}\nTargets not yet defined: ${_targetsNotDefined}\n")
endif()

unset(_targetsDefined)
unset(_targetsNotDefined)
unset(_expectedTargets)

# Compute the installation prefix relative to this file.
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)

if(_IMPORT_PREFIX STREQUAL "/")
    set(_IMPORT_PREFIX "")
endif()

# Create imported target artf::dtcore
add_library(artf::dtcore STATIC IMPORTED GLOBAL)
set_target_properties(artf::dtcore PROPERTIES
    IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libdtcore.a
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
    INTERFACE_LINK_LIBRARIES "yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf" # "yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf"
    INTERFACE_COMPILE_FEATURES "cxx_std_17"
    LINKER_LANGUAGE CXX
)

add_library(artf::dtcore_grpc STATIC IMPORTED GLOBAL)
set_target_properties(artf::dtcore_grpc PROPERTIES
    IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libdtcore_grpc.a
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
    INTERFACE_LINK_LIBRARIES "yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf;gRPC::grpc++;gRPC::grpc++_reflection" # "yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf"
    INTERFACE_COMPILE_FEATURES "cxx_std_17"
    LINKER_LANGUAGE CXX
)

# Create imported target artf::proto
add_library(artf::dtproto SHARED IMPORTED GLOBAL)
set_target_properties(artf::dtproto PROPERTIES
    IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libdtproto.a
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include;${_IMPORT_PREFIX}/include"
    INTERFACE_LINK_LIBRARIES "yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf" # "yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf;gRPC::grpc++;gRPC::grpc++_reflection"
    INTERFACE_COMPILE_FEATURES "cxx_std_17"
    LINKER_LANGUAGE CXX
)

add_library(artf::dtproto_grpc SHARED IMPORTED GLOBAL)
set_target_properties(artf::dtproto_grpc PROPERTIES
    IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libdtproto_grpc.a
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include;${_IMPORT_PREFIX}/include"
    INTERFACE_LINK_LIBRARIES "yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf;gRPC::grpc++;gRPC::grpc++_reflection" # "yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf;gRPC::grpc++"
    INTERFACE_COMPILE_FEATURES "cxx_std_17"
    LINKER_LANGUAGE CXX
)

# Cleanup temporary variables.
set(_IMPORT_PREFIX)

# Loop over all imported files and verify that they actually exist
foreach(target ${_IMPORT_CHECK_TARGETS})
    foreach(file ${_IMPORT_CHECK_FILES_FOR_${target}})
        if(NOT EXISTS "${file}")
            message(FATAL_ERROR "The imported target \"${target}\" references the file
   \"${file}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty and contained
   \"${CMAKE_CURRENT_LIST_FILE}\"
but not all the files it references.
")
        endif()
    endforeach()

    unset(_IMPORT_CHECK_FILES_FOR_${target})
endforeach()

unset(_IMPORT_CHECK_TARGETS)

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
cmake_policy(POP)