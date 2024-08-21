# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

cmake_minimum_required ( VERSION 2.8...3.5 )

# Project setup

set ( APPLICATION_ID fedemKernel )
set ( DOMAIN_ID FEDEM )
set ( PACKAGE_ID FOUNDATION )

option ( USE_FORTRAN "Build with Fortran code included" OFF )
option ( USE_VTFAPI "Build with VTF API included" OFF )
option ( USE_MEMPOOL "Use memory pool for heap allocation in FE library" OFF )
option ( USE_PROFILER "Use CPU and Memory profiler" OFF )
mark_as_advanced ( USE_FORTRAN USE_MEMPOOL USE_PROFILER )

if ( USE_FORTRAN)
  project ( ${APPLICATION_ID} CXX Fortran )
else ( USE_FORTRAN)
  project ( ${APPLICATION_ID} CXX )
endif ( USE_FORTRAN)
message ( STATUS "Generating build project for ${PROJECT_SOURCE_DIR}" )
find_path ( _MODULES FedemConfig.cmake
            PATHS $ENV{CMAKE_MODULES}
           "${PROJECT_SOURCE_DIR}/cmake/Modules/"
          )
if ( _MODULES )
  message ( STATUS "NOTE : Using ${_MODULES}" )
  list ( APPEND CMAKE_MODULE_PATH ${_MODULES} )
else ( _MODULES )
  message ( STATUS "ERROR : Missing path to FedemConfig.cmake" )
  message ( FATAL_ERROR "Set environment variable CMAKE_MODULES and try again" )
endif ( _MODULES )
unset ( _MODULES CACHE )

include ( CheckPFUnit )
include ( FedemConfig )

if ( BUILD_TESTS )

# Enable regression/unit testing

  enable_testing ()
  set ( CTEST_OPTIONS --force-new-ctest-process --output-on-failure -O CTest.txt )
  if ( CMAKE_CONFIGURATION_TYPES )
    list ( APPEND CTEST_OPTIONS --build-config \"$<CONFIGURATION>\" )
  endif ( CMAKE_CONFIGURATION_TYPES )
  add_custom_target ( check COMMAND ${CMAKE_CTEST_COMMAND} ${CTEST_OPTIONS} )

  include ( GTest ) # Using the google test framework for C++ unit tests
  if ( USE_FORTRAN )
    include ( pFUnit ) # Using the pFUnit test framework for Fortran unit tests
  endif ( USE_FORTRAN )

endif ( BUILD_TESTS )

add_subdirectory ( src )
