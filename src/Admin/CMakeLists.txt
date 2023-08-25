# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID Admin )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )

include ( Date ) # Macro for build date handling

Date ( "build_date" "${CMAKE_CURRENT_BINARY_DIR}" ) # Generate new build date

# Extract new build number as the patch number from the file "cfg/VERSION"
# and write it out to "build_no.h" in the build directory
find_file ( VERSION "VERSION" PATHS ${PROJECT_SOURCE_DIR}/cfg
                                    ${CONFIG_DIR} NO_DEFAULT_PATH )
if ( VERSION )
  message ( STATUS "INFORMATION : Extracting build number from ${VERSION}" )
  file ( READ ${VERSION} BUILD_NO )
  string ( REGEX REPLACE "^.*\\." "" BUILD_NO ${BUILD_NO} )
  execute_process ( COMMAND "${CMAKE_COMMAND}" -E echo_append
                    "-- INFORMATION : Build number ${BUILD_NO}" )
  file ( WRITE "${CMAKE_CURRENT_BINARY_DIR}/build_no.h" ${BUILD_NO} )
else ( VERSION )
  message ( WARNING "No VERSION file found. Setting build number to zero." )
  file ( WRITE "${CMAKE_CURRENT_BINARY_DIR}/build_no.h" 0 )
endif ( VERSION )
include_directories ( "${CMAKE_CURRENT_BINARY_DIR}" )

if ( PLATFORM_BITSIZE_IS_64_BIT )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_IS64BIT" )
endif ( PLATFORM_BITSIZE_IS_64_BIT )
if ( FTENV_WARNINGS AND LINUX AND NOT USE_INTEL_FORTRAN )
  string ( APPEND CMAKE_CXX_FLAGS " -Wno-stringop-truncation" )
endif ( FTENV_WARNINGS AND LINUX AND NOT USE_INTEL_FORTRAN )

add_library ( ${LIB_ID} FedemAdmin.C FedemAdmin.H version.h )
