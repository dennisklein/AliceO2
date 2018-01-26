cmake_minimum_required(VERSION 3.9.4 FATAL_ERROR)

# Define the project, the version and enable languages
project(ALICEO2 VERSION 0.0.0 LANGUAGES CXX)

# Add our modules to the search path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Find the project dependencies
include(${CMAKE_SOURCE_DIR}/cmake/O2Dependencies.cmake)

# Load some basic macros which are needed later on
include(${CMAKE_SOURCE_DIR}/cmake/O2Utils.cmake)
include(FairRootMacros)
include(CTest)

# Abort if the build directory is the source directory
check_out_of_source_build()

# Override FairRoot's default build type
if(NOT CMAKE_BUILD_TYPE)
  Message(STATUS "Set BuildType DEBUG")
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Override FairRoot's default CXXSTD
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# Define FairRoot build types and perform various compiler checks
fairroot_setup_compiler()

# Add build type for coverage builds
set(CMAKE_CXX_FLAGS_COVERAGE "-g -O2 -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_COVERAGE}")
set(CMAKE_Fortran_FLAGS_COVERAGE "-g -O2 -fprofile-arcs -ftest-coverage")
set(CMAKE_LINK_FLAGS_COVERAGE "--coverage -fprofile-arcs  -fPIC")
 
# Check C++11 features
set(CheckSrcDir "${FairRoot_CHECKS_DIR}")
include(CheckCXX11Features)

# Check C++14 features
set(CheckCXX14SrcDir "${CMAKE_SOURCE_DIR}/cmake/checks")
include(CheckCXX14Features)

# Check, if we have a simulation environment
if(${Geant4_FOUND} AND ${Geant3_FOUND} AND ${Geant4VMC_FOUND})
  set(HAVESIMULATION 1)
  message(STATUS "Simulation environment found")
endif()

# TODO
# set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
# set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
# set(INCLUDE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/include")
# Set(VMCWORKDIR ${CMAKE_SOURCE_DIR})
# SET(_LIBDIR ${CMAKE_BINARY_DIR}/lib)
# SET(LD_LIBRARY_PATH ${_LIBDIR} ${LD_LIBRARY_PATH})

# Define targets (e.g. libraries, executables) to build
add_custom_target(man ALL)
add_subdirectory(Generators)
# set(GENERATORS_LIBRARY Generators)
#
# add_subdirectory(CCDB)
# add_subdirectory(Common)
# add_subdirectory(DataFormats)
# add_subdirectory(Detectors)
# add_subdirectory(EventVisualisation)
# add_subdirectory(Examples)
# add_subdirectory(Framework)
# add_subdirectory(Algorithm)
# add_subdirectory(macro)
# add_subdirectory(Utilities)
# add_subdirectory(Steer)
# add_subdirectory(doc)
# if (HAVESIMULATION)
#    add_subdirectory(run)
# endif()
# add_subdirectory(config)
#
# IF (IWYU_FOUND)
#   ADD_CUSTOM_TARGET(checkHEADERS
#       DEPENDS $ENV{ALL_HEADER_RULES}
#       )
# ENDIF ()
#
# Install(FILES ${CMAKE_BINARY_DIR}/check_system.sh
#         DESTINATION bin
#        )
#
# SET(VMCWORKDIR ${CMAKE_SOURCE_DIR})
#
# WRITE_CONFIG_FILE(config.sh)
#
# SET(VMCWORKDIR ${CMAKE_INSTALL_PREFIX}/share)
# SET(ROOT_INCLUDE_PATH ${CMAKE_INSTALL_PREFIX}/include)
# WRITE_CONFIG_FILE(config.sh_install)
#
# Install(FILES ${CMAKE_BINARY_DIR}/config.sh_install
#         DESTINATION bin
#         RENAME O2Config.sh
#        )
#
# # Place the CTestCustom.cmake in the build dir
# configure_file(${CMAKE_SOURCE_DIR}/CTestCustom.cmake
#     ${CMAKE_BINARY_DIR}/CTestCustom.cmake
#     )
#
# O2_GENERATE_MAN(NAME o2)
# O2_GENERATE_MAN(NAME FairMQDevice)
#
# # Run macros unit tests automatically if ALIBUILD_O2_TESTS is defined
# IF($ENV{ALIBUILD_O2_TESTS})
#   SET(CHECK_ROOTMACRO_COMPILE 1)
# ENDIF()
#
# # UNIT TESTS VERIFYING CONSISTENT STATE OF OUR ROOT MACROS AND THE EXECUTION ENVIRONMENT
# if(CHECK_ROOTMACRO_COMPILE AND HAVESIMULATION)
#   FIND_PROGRAM(BASH_PROGRAM bash)
#
#   # On Mac OS GLOB_RECURSE returns both .C and .c files, i.e. case insensitive
#   FILE(GLOB_RECURSE MACRO_FILES "*.C")
#   # Case sensitive filtering of .C files
#   LIST(FILTER MACRO_FILES INCLUDE REGEX "^.*\\.C$")
#   SET(TEST_NO 0)
#   FOREACH(MACRO_FILE ${MACRO_FILES})
#     MATH(EXPR TEST_NO "${TEST_NO} + 1")
#     SET(EXE_NAME ${CMAKE_BINARY_DIR}/tmp_scripts/macro_test${TEST_NO}.sh)
#
#     # Create test script for macros
#     FILE(WRITE ${EXE_NAME} "export LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${CMAKE_BINARY_DIR}/lib\n root -b -l -q -e \".L ${MACRO_FILE}\"")
#
#     ADD_TEST(NAME ${MACRO_FILE}
#       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#       COMMAND ${BASH_PROGRAM} ${EXE_NAME}
#     )
#   ENDFOREACH()
# endif()

message("Compiling ${BWhite}${PROJECT_NAME}${CR} on ${BWhite}${FAIRROOT_SYSTEM}/${FAIRROOT_COMPILER}${CR} with build type ${BWhite}${CMAKE_BUILD_TYPE}${CR}")
