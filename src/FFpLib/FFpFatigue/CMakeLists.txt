# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFpFatigue )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFpCycle FFpFatigue FFpSNCurve FFpSNCurveLib
                          FFpDamageAccumulator )
## Pure implementation files, i.e., source files without corresponding header
set ( SOURCE_FILE_LIST )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
  set ( SOURCE_FILE_LIST FFpFatigue_F )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )

foreach ( FILE ${COMPONENT_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
  list ( APPEND CPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${COMPONENT_FILE_LIST} )

foreach ( FILE ${SOURCE_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
endforeach ( FILE ${SOURCE_FILE_LIST} )


add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} FFaString FFaDefinitions )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
  target_link_libraries ( ${LIB_ID} FFaCmdLineArg )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
