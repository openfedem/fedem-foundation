# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFaFunction )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


# Build and install

add_library ( ${LIB_ID} SHARED FFaFunction.C FFaFunction.h )
target_link_libraries ( ${LIB_ID} LINK_PRIVATE FFaFunctionLib Admin )

install ( TARGETS ${LIB_ID}
          RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
          LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" )
