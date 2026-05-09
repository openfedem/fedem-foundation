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
add_executable ( cad2vtf cad2vtf.C )
target_link_libraries ( ${LIB_ID} FFlIOAdaptors )
target_link_libraries ( fem2vtf FFlIOAdaptors )
target_link_libraries ( cad2vtf FFlIOAdaptors )
if ( USE_FORTRAN_INTERFACE )
  add_executable ( test_FFlMemChk testMemchk.f90 )
  target_link_libraries ( test_FFlMemChk FFlLib_F )
endif ( USE_FORTRAN_INTERFACE )

if ( pFUnit_FOUND AND USE_FORTRAN_INTERFACE )
# Add some unit tests using pFUnit (executed via ctest)
  enable_fortran_tests ()
  unset ( CMAKE_Fortran_MODULE_DIRECTORY )
  include_directories ( ${PROJECT_BINARY_DIR}/f90_modules )
  add_fortran_test ( test_spring testSpring.pf FFlLib_F )
endif ( pFUnit_FOUND AND USE_FORTRAN_INTERFACE )

if ( GTest_FOUND )
# Add some unit tests using gtest (executed via ctest)
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_HAS_GTEST" )
  add_executable ( test_invert testInvert.C )
  add_cpp_test ( test_invert FFlLib )
  add_executable ( test_parser testParser.C )
  add_cpp_test ( test_parser FFlIOAdaptors )
  add_executable ( test_sections testXsections.C )
  add_cpp_test ( test_sections FFlIOAdaptors )
endif ( GTest_FOUND )

if ( TARGET FFlVisualization )
  add_executable ( test_FFlViz testFFlVis.C )
  if ( GTest_FOUND )
    add_cpp_test ( test_FFlViz FFlVisualization FFlIOAdaptors )
  else ( GTest_FOUND )
    target_link_libraries ( test_FFlViz FFlVisualization FFlIOAdaptors )
  endif ( GTest_FOUND )
endif ( TARGET FFlVisualization )
