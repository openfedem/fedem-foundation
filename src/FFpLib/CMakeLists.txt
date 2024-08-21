# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFpLib )
set ( LIB_ID_LIST FFpCurveData FFpFatigue )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  list ( APPEND LIB_ID_LIST FFpExport FFpDLL )
  if ( BUILD_TESTS )
    list ( APPEND LIB_ID_LIST FFpTests )
  endif ( BUILD_TESTS )
else ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  string ( APPEND CMAKE_CXX_FLAGS " -DFFR_LISTVIEW" )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )

foreach ( FOLDER ${LIB_ID_LIST} )
  add_subdirectory ( ${FOLDER} )
endforeach ( FOLDER ${LIB_ID_LIST} )


## Fortran 90 source files
set ( F90_FILE_LIST FFpExport/FFpBatchExportInterface FFpFatigue/FFpFatigueInterface )

foreach ( FILE ${F90_FILE_LIST} )
  list ( APPEND F90_SOURCE_FILES ${FILE}.f90 )
endforeach ( FILE ${F90_FILE_LIST} )


if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
  add_library ( ${LIB_ID}_F ${F90_SOURCE_FILES} )
  target_link_libraries ( ${LIB_ID}_F FFpExport FFpFatigue )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
