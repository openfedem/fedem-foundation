# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FiDeviceFunctions )
set ( LIB_ID_LIST )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND BUILD_TESTS )
  set ( LIB_ID_LIST FiDeviceFunctionsTests )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND BUILD_TESTS )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


# Subfolder handling

foreach ( FOLDER ${LIB_ID_LIST} )
  add_subdirectory ( ${FOLDER} )
endforeach ( FOLDER ${LIB_ID_LIST} )

if ( LINUX )
  string ( APPEND CMAKE_CXX_FLAGS " -Wno-unused-result" )
endif ( LINUX )

## Files with header and source with same name
set ( COMPONENT_FILE_LIST FiASCFile FiCurveASCFile FiDACFile
                          FiDeviceFunctionBase FiDeviceFunctionFactory
                          FiRAOTable FiRPC3File )
## Pure header files, i.e., header files without a corresponding source file
set ( HEADER_FILE_LIST FiSwappedIO )
## Pure implementation files, i.e., source files without corresponding header
set ( SOURCE_FILE_LIST )
if ( USE_FORTRAN )
  set ( SOURCE_FILE_LIST FiDeviceFunctionFactory_F )
  if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
    list ( APPEND SOURCE_FILE_LIST FiRAOTable_F )
  endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
endif ( USE_FORTRAN )
## Fortran 90 source files
set ( F90_FILE_LIST fiDeviceFunctionInterface fiRAOTableInterface )


foreach ( FILE ${COMPONENT_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
  list ( APPEND CPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${COMPONENT_FILE_LIST} )

foreach ( FILE ${HEADER_FILE_LIST} )
  list ( APPEND CPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${HEADER_FILE_LIST} )

foreach ( FILE ${SOURCE_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
endforeach ( FILE ${SOURCE_FILE_LIST} )

foreach ( FILE ${F90_FILE_LIST} )
  list ( APPEND F90_SOURCE_FILES ${FILE}.f90 )
endforeach ( FILE ${F90_FILE_LIST} )

add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} FFaDefinitions FFaOS )

if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
  add_library ( ${LIB_ID}_F90 ${F90_SOURCE_FILES} )
  target_link_libraries ( ${LIB_ID}_F90 ${LIB_ID} )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
