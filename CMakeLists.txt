cmake_minimum_required(VERSION 3.13)

project(dtCore
    LANGUAGES C CXX
    HOMEPAGE_URL https://github.hmc.co.kr/art_framework
    DESCRIPTION "HMC's Art Framework Library"
    VERSION 1.12.0
)

cmake_policy(VERSION 3.13)
cmake_policy(SET CMP0077 NEW) # option() honors normal variables

# generate version.h 
set(version_h_in ${CMAKE_CURRENT_SOURCE_DIR}/include/dtCore/version.h.in)
set(version_h ${CMAKE_CURRENT_SOURCE_DIR}/include/dtCore/version.h)
configure_file(${version_h_in} ${version_h} @ONLY)

# set compiler options
#add_compile_options( -g -Wall )
#set(CMAKE_C_FLAGS "-Wall -std=c99")
#set(CMAKE_C_FLAGS_DEBUG "-Wall -g")
#set(CMAKE_C_FLAGS_RELEASE "-Wall -O4 -DNDEBUG")
#set(CMAKE_CXX_FLAGS "-Wall -std=c++0x -stdlib=libc++")
#set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g")
#set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O4 -DNDEBUG")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OS specific build options
if(WIN32)
elseif(APPLE) # APPLE should be tested before UNIX (OSX also set UNIX true)
  message(STATUS "APPLE")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "12.7")
elseif(UNIX) # for *Nix
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

if("$ENV{ARTF_INSTALL_DIR}" STREQUAL "")
    set(ARTF_INSTALL_DIR "$ENV{HOME}/.local/ART_Framework")
else()
    file(TO_CMAKE_PATH "$ENV{ARTF_INSTALL_DIR}" ARTF_INSTALL_DIR)
endif()

if("${ARTF_INSTALL_DIR}" STREQUAL "")
    message(FATAL_ERROR "ARTF_INSTALL_DIR is not set.")
else()
    message(STATUS "ARTF_INSTALL_DIR: ${ARTF_INSTALL_DIR}")
endif()

set(CMAKE_PREFIX_PATH "${ARTF_INSTALL_DIR};${CMAKE_PREFIX_PATH}")
set(CMAKE_INSTALL_PREFIX "${ARTF_INSTALL_DIR}")

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

set(PROTOBUF_GEN_DIR "") # this variable will be set while configuring 'src' directory

# --------------------------------------------------------
# Build options
# use it that way cmake .. -DBUILD_EXAMPLES=ON -DBUILD_dtProto=ON
# --------------------------------------------------------
OPTION(BUILD_DOCS           "Build documents"                         OFF)
OPTION(BUILD_UNIT_TESTS     "Build unit test"                         OFF)
OPTION(BUILD_EXAMPLES       "Build examples"                          OFF)
OPTION(BUILD_EXAMPLES_eCAL  "Build eCAL examples"                     OFF)
OPTION(BUILD_EXAMPLES_gRPC  "Build gRPC examples"                     OFF)
OPTION(BUILD_EXAMPLES_MCAP  "Build MCAP examples"                     OFF)
OPTION(BUILD_dtCore         "Build dtCore library"                    ON)
OPTION(BUILD_dtProto        "Build dtProto library"                   ON)
OPTION(BUILD_dtProto_gRPC   "Build dtProto library with gRPC support" ON)
OPTION(BUILD_dtCore_gRPC    "Build dtCore with gRPC DAQ support"      ON)


# --------------------------------------------------------
# Find & Include 3rd-party modules
# --------------------------------------------------------
# include(cmake/common.cmake)

# DOWNLOAD ALL THE SUBMODULES
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update...")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules.")
        endif()
    endif()
endif()

# CHECK & BUILD SUBMODULES
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extlib/spdlog/CMakeLists.txt")
    message(FATAL_ERROR "The spdlog submodules was not downloaded. GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extlib/yaml-cpp/CMakeLists.txt")
    message(FATAL_ERROR "The yaml-cpp submodules was not downloaded. GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

set(SPDLOG_INSTALL ON)
add_subdirectory(extlib/spdlog)
set(YAML_CPP_INSTALL ON)
add_subdirectory(extlib/yaml-cpp)

# --------------------------------------------------------
# Build dtCore modules
# --------------------------------------------------------
add_subdirectory(src)

# --------------------------------------------------------
# Build unit tests
# --------------------------------------------------------
if(BUILD_UNIT_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()


# --------------------------------------------------------
# Build examples
# --------------------------------------------------------
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()


# --------------------------------------------------------
# Build documents
# --------------------------------------------------------
macro(path_linux_to_win MsysPath ResultingPath)
  string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()
macro(path_win_to_linux MsysPath ResultingPath)
  string(REGEX REPLACE "^([a-zA-Z]):/" "/\\1/" ${ResultingPath} "${MsysPath}")
endmacro()

if(BUILD_DOCS)
  find_package(Doxygen 1.8.17)
  message(STATUS "Can build doc?                            ${DOXYGEN_FOUND}")
  if (DOXYGEN_FOUND)
      # Search for plantUML for creating UML diagrams from doxygen
      find_program(PLANT_UML_PATH plantuml.jar PATH_SUFFIXES PlantUML plantuml Plantuml 
          PATHS /usr/share /usr/local/share/ /usr/local/bin c/Program\ Files* )
      if(NOT PLANT_UML_PATH)
          message(WARNING "Looking for PlantUML - not found, some UML diagrams will not be generated via doxygen.")
      else()
          message(STATUS "  + PlantUML - for custom UML             YES ")
      endif()

      #Search for DOT for autogenerated UML diagrams from doxygen
      find_program(DOT_PATH dot PATH_SUFFIXES graphviz2.38/bin graphviz/bin)
      if(NOT DOT_PATH)
          message(WARNING "Looking for DOT (Graphviz) - not found, some UML diagrams will not be generated via doxygen.")
      else()
          message(STATUS "  + Graphviz/Dot - for generated graphs   YES ")
      endif()

      path_win_to_linux(${CMAKE_CURRENT_SOURCE_DIR} CMAKE_CURRENT_SOURCE_DIR_LINUX)
      path_win_to_linux(${PROJECT_SOURCE_DIR} PROJECT_SOURCE_DIR_LINUX)
      path_win_to_linux(${DOT_PATH} DOT_PATH_LINUX)
      path_win_to_linux(${PLANT_UML_PATH} PLANT_UML_PATH_LINUX)
      path_win_to_linux(${CMAKE_CURRENT_BINARY_DIR} CMAKE_CURRENT_BINARY_DIR_LINUX)

      # configure doxygen configuration file
      set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
      set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
      configure_file(${doxyfile_in} ${doxyfile} @ONLY)

      # doc build only target, target is not in default build, so it must be
      # triggered explicitly
      add_custom_target(doc
          COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
          COMMENT "Generating API documentation with Doxygen"
          VERBATIM)

        # TODO: Where docs will be installed.
        # Note: if docs shall be not a part of final config, delete this install command,
        # docs will be only available in [build_dir]/doc/doc/index.html
        install(
            DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/
            DESTINATION ./share/doc/${PROJECT_NAME}
            OPTIONAL)
  else(DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
  endif(DOXYGEN_FOUND)
endif(BUILD_DOCS)


# --------------------------------------------------------
# Install
# --------------------------------------------------------
install(DIRECTORY ./include/dtCore
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    artfConfigVersion.cmake
    VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
    COMPATIBILITY ExactVersion)

configure_file(cmake/artfConfig.cmake.in artfConfig.cmake @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/artfConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/artfConfigVersion.cmake"
    "cmake/artfTargets.cmake"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/artf)

# --------------------------------------------------------
# Finalize configuration and print options
# --------------------------------------------------------
message(STATUS "BUILD_DOCS                                     : ${BUILD_DOCS}")
message(STATUS "BUILD_UNIT_TESTS                               : ${BUILD_UNIT_TESTS}")
message(STATUS "BUILD_EXAMPLES                                 : ${BUILD_EXAMPLES}")
message(STATUS "BUILD_EXAMPLES_eCAL                            : ${BUILD_EXAMPLES_eCAL}")
message(STATUS "BUILD_EXAMPLES_gRPC                            : ${BUILD_EXAMPLES_gRPC}")
message(STATUS "BUILD_EXAMPLES_MCAP                            : ${BUILD_EXAMPLES_MCAP}")
message(STATUS "BUILD_dtCore                                   : ${BUILD_dtCore}")
message(STATUS "BUILD_dtProto                                  : ${BUILD_dtProto}")
message(STATUS "BUILD_dtProto_gRPC                             : ${BUILD_dtProto_gRPC}")
message(STATUS "BUILD_dtCore_gRPC                              : ${BUILD_dtCore_gRPC}")
