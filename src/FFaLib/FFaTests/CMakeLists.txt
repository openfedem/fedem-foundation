# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Build setup

set ( LIB_ID FFaTests )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


if ( GTest_FOUND )
# Add some unit tests using gtest (executed via ctest)
  add_executable ( test_FFa test_FFa.C BodyTest.C )
  add_executable ( test_FFaEnum test_FFaEnum.C )
  add_executable ( test_FFaVersion test_FFaVersion.C )
  add_cpp_test ( test_FFa FFaAlgebra )
  add_cpp_test ( test_FFaEnum FFaString )
  add_cpp_test ( test_FFaVersion FFaDefinitions )
endif ( GTest_FOUND )

# Old-style unit tests (remove when all are covered by gtest)
add_executable ( ${LIB_ID} FFaTest.C BodyTest.C GeometryTest.C )
target_link_libraries ( ${LIB_ID} FFaAlgebra FFaCmdLineArg )
if ( TARGET FFaGeometry )
  target_link_libraries ( ${LIB_ID} FFaGeometry )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_HAS_GEOMETRY" )
endif ( TARGET FFaGeometry )
