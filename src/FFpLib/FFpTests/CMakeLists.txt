# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Build setup

set ( LIB_ID FFpTests )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )

if ( GTest_FOUND )
  add_executable ( test_FFp test_FFp.C )
  add_cpp_test ( test_FFp FFpCurveData FFrLib )
endif ( GTest_FOUND )
