# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Build setup

set ( LIB_ID FiDeviceFunctionsTests )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )

set ( TESTS RAOTable ASCFile )

if ( GTest_FOUND )
# Add some unit tests using gtest (executed via ctest)
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_HAS_GTEST" )
  add_executable ( test_FiDF ReaderTest.C )
  add_cpp_test ( test_FiDF FiDeviceFunctions FFaString )
endif ( GTest_FOUND )

# Old-style unit tests
foreach ( TEST ${TESTS} )
  add_executable ( test_${TEST} ${TEST}Test.C )
  target_link_libraries ( test_${TEST} FiDeviceFunctions )
endforeach ( TEST ${TESTS} )
