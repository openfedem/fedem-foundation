# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Build setup

set ( LIB_ID FFlTests )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )

add_executable ( ${LIB_ID} main.C convertToFtl.C )
add_executable ( fem2vtf fem2vtf.C )
add_executable ( test_fflmemchk testMemchk.f90 )
target_link_libraries ( ${LIB_ID} FFlLib )
target_link_libraries ( fem2vtf FFlLib )
target_link_libraries ( test_fflmemchk FFlLib_F )

if ( pFUnit_FOUND )
# Add some unit tests using pFUnit (executed via ctest)
  enable_fortran_tests ()
  unset ( CMAKE_Fortran_MODULE_DIRECTORY )
  include_directories ( ${PROJECT_BINARY_DIR}/f90_modules )
  add_fortran_test ( test_spring testSpring.pf FFlLib_F )
endif ( pFUnit_FOUND )

if ( GTest_FOUND )
# Add some unit tests using gtest (executed via ctest)
  add_executable ( test_invert testInvert.C )
  add_cpp_test ( test_invert FFlLib )
  add_executable ( test_FEparser testParser.C )
  add_cpp_test ( test_FEparser FFlLib FFaCmdLineArg )
endif ( GTest_FOUND )
