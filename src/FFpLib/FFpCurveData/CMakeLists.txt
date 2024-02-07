# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFpCurveData )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


if ( USE_MEMPOOL )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_MEMPOOL" )
endif ( USE_MEMPOOL )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFpCurve FFpCurveDef FFpFourier
                          FFpGraph FFpReadResults )
## Pure header files, i.e., header files without a corresponding source file
set ( HEADER_FILE_LIST FFpDFTparams )


foreach ( FILE ${COMPONENT_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
  list ( APPEND CPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${COMPONENT_FILE_LIST} )

foreach ( FILE ${HEADER_FILE_LIST} )
  list ( APPEND CPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${HEADER_FILE_LIST} )


set ( DEPENDENCY_LIST FFaMathExpr FFpFatigue FiDeviceFunctions
                      FFaDefinitions FFaOperation )

add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} ${DEPENDENCY_LIST} )
