# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Build setup

set ( LIB_ID FFrTests )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


if ( WIN )
  unset ( DIRENT_FOUND CACHE )
  find_file ( DIRENT_FOUND dirent.h )
  if ( DIRENT_FOUND )
    string ( APPEND CMAKE_CXX_FLAGS " -DFT_HAS_DIRENT" )
    message ( STATUS "Found Dirent API: ${DIRENT_FOUND}" )
  elseif ( FTENV_VERBOSE )
    message ( STATUS "INFORMATION : The Dirent API is not available." )
    message ( STATUS "              You can install it from http://softagalleria.net" )
  endif ( DIRENT_FOUND )
else ( WIN )
# Always assuming dirent.h is present on Linux
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_HAS_DIRENT" )
endif ( WIN )

add_executable ( test_FFr FFrTest.C loadTest.C )
if ( GTest_FOUND )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_HAS_GTEST" )
  target_sources ( test_FFr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/searchTest.C )
  add_cpp_test ( test_FFr FFrLib )
else ( GTest_FOUND )
  target_link_libraries ( test_FFr FFrLib )
endif ( GTest_FOUND )
