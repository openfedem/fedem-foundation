# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFp )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


file ( GLOB CPP_SOURCE_FILES *.C )
file ( GLOB CPP_HEADER_FILES *.h )

# Build and install

add_library ( ${LIB_ID} SHARED ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} LINK_PRIVATE FFpFatigue FFpCurveData FFaOperation FFrLib )

install ( TARGETS ${LIB_ID}
          RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
          LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" )
