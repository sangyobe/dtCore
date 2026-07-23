include(DebugUtil)
include(GetDirectoryName)

include_directories(
    "${CMAKE_SOURCE_DIR}/extlib/spdlog/include"
    "${CMAKE_SOURCE_DIR}/extlib/yaml-cpp/include"
    "${CMAKE_SOURCE_DIR}/extlib/mcap/cpp/mcap/include"
    "${CMAKE_SOURCE_DIR}/include"
)
link_directories(
    "${CMAKE_SOURCE_DIR}/lib"
)

# ─────────────────────────────────────────────────────────────────────────────
# Local-build redirect: when examples are built inside the dtcore project tree,
# point every example's find_package(artf CONFIG REQUIRED) at the locally built
# targets instead of the installed package.
#
# Mechanism:
#  1) Create artf:: ALIAS targets for each locally built target (dtcore, …).
#  2) Write a minimal shim artfConfig.cmake into the build tree that only sets
#     artf_FOUND TRUE (no target import — the aliases above serve that role).
#  3) Force artf_DIR (CMake cache) to the shim directory so subdirectory calls
#     to find_package(artf) resolve the shim first, even when artf_DIR was
#     previously cached to the installed location.
#
# artfTargets.cmake from the installed package has its own guard:
#   "if all expected artf:: targets are already defined → return()"
# so even if the installed config file is somehow reached it is a no-op.
#
# When examples are built standalone (TARGET dtcore is not defined) this block
# is skipped and find_package(artf) falls back to the installed package as usual.
# ─────────────────────────────────────────────────────────────────────────────
if(TARGET dtcore)
    foreach(_local dtcore dtcore_grpc dtproto dtproto_grpc)
        if(TARGET ${_local} AND NOT TARGET artf::${_local})
            add_library(artf::${_local} ALIAS ${_local})
        endif()
    endforeach()
    unset(_local)

    set(_ARTF_SHIM "${CMAKE_BINARY_DIR}/artf_shim/lib/cmake/artf")
    file(MAKE_DIRECTORY "${_ARTF_SHIM}")
    file(WRITE "${_ARTF_SHIM}/artfConfig.cmake"
        "# Local-build shim — artf:: targets are already defined as ALIAS targets\n"
        "# by examples/CMakeLists.txt; nothing to import here.\n"
        "set(artf_FOUND TRUE)\n"
        "set_target_properties(dtcore PROPERTIES\n"
        "    INTERFACE_LINK_LIBRARIES \"yaml-cpp::yaml-cpp;spdlog::spdlog;protobuf::libprotobuf\"\n"
        ")\n"
    )
    file(WRITE "${_ARTF_SHIM}/artfConfigVersion.cmake"
        "set(PACKAGE_VERSION \"999.0.0\")\n"
        "set(PACKAGE_VERSION_COMPATIBLE TRUE)\n"
        "set(PACKAGE_VERSION_EXACT FALSE)\n"
    )
    set(artf_DIR "${_ARTF_SHIM}" CACHE PATH
        "artf cmake config dir (local-build shim, managed by examples/CMakeLists.txt)" FORCE)
    unset(_ARTF_SHIM)
endif()

if(BUILD_dtProto)
  include_directories("${PROTOBUF_GEN_DIR}")
endif()

# Mcap example targets
if(BUILD_dtProto AND BUILD_EXAMPLES_MCAP)
  file(GLOB EXAMPLE_DIRS "example_mcap_*")
  foreach(example_dir ${EXAMPLE_DIRS})
      add_subdirectory(${example_dir})
  endforeach()
endif()

# eCAL example targets
if(BUILD_dtProto AND BUILD_EXAMPLES_eCAL)
  file(GLOB EXAMPLE_DIRS "example_ecal_*")
  foreach(example_dir ${EXAMPLE_DIRS})
      add_subdirectory(${example_dir})
  endforeach()
endif()

# gRPC example targets
if(BUILD_dtProto AND BUILD_dtProto_gRPC AND BUILD_EXAMPLES_gRPC)
  file(GLOB EXAMPLE_DIRS "example_grpc_*")
  foreach(example_dir ${EXAMPLE_DIRS})
      add_subdirectory(${example_dir})
  endforeach()
endif()

# other examples
file(GLOB EXAMPLE_DIRS "example_thread*" "example_log*" "example_rtlog*" "example_utils*")

foreach(example_dir ${EXAMPLE_DIRS})
    add_subdirectory(${example_dir})
endforeach()
