# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Build setup

set ( LIB_ID FFaMathExprTests )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


if ( GTest_FOUND )
# Add some unit tests using gtest (executed via ctest)
  add_executable ( test_FFaMathExpr test_FFaMathExpr.C eval_expression.C )
  add_cpp_test ( test_FFaMathExpr FFaMathExpr )
else ( GTest_FOUND )
# Old-style unit test, built only when google test is not installed
  add_executable ( ${LIB_ID} main.C eval_expression.C )
  target_link_libraries ( ${LIB_ID} FFaMathExpr )
endif ( GTest_FOUND )
