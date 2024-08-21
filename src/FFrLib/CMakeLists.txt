# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFrLib )
set ( LIB_ID_LIST )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND BUILD_TESTS )
  set ( LIB_ID_LIST FFrTests )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND BUILD_TESTS )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  string ( APPEND CMAKE_CXX_FLAGS_DEBUG " -DFFR_DEBUG=1" )
else ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  string ( APPEND CMAKE_CXX_FLAGS " -DFFR_LISTVIEW" )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )

if ( USE_PROFILER )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_PROFILER" )
endif ( USE_PROFILER )

if ( USE_MEMPOOL )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_MEMPOOL" )
endif ( USE_MEMPOOL )

if ( USE_LOWLEVEL_IO )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_LOWLEVEL_IO" )
endif ( USE_LOWLEVEL_IO )

string ( APPEND CMAKE_CXX_FLAGS " -DFFR_NEWALLOC" )

foreach ( FOLDER ${LIB_ID_LIST} )
  add_subdirectory ( ${FOLDER} )
endforeach ( FOLDER ${LIB_ID_LIST} )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFrEntryBase FFrExtractor
                          FFrFieldEntryBase FFrItemGroup FFrObjectGroup
                          FFrResultContainer FFrSuperObjectGroup
                          FFrVariable FFrVariableReference
)

## Pure header files, i.e., header files without a corresponding source file
set ( HEADER_FILE_LIST FFrReadOp FFrReadOpInit )

## Pure implementation files, i.e., source files without corresponding header
set ( SOURCE_FILE_LIST FFrReadOpImpl )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  list ( APPEND SOURCE_FILE_LIST FFrExtractor_F )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )

## Fortran 90 source files
set ( F90_FILE_LIST FFrExtractorInterface )


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


#
# Dependencies and system libraries
#
set ( DEPENDENCY_LIST FFaAlgebra FFaString FFaDefinitions FFaOS )

if ( USE_MEMPOOL )
  list ( APPEND DEPENDENCY_LIST FFaPatterns )
endif ( USE_MEMPOOL )

if ( USE_PROFILER )
  list ( APPEND DEPENDENCY_LIST FFaProfiler )
endif ( USE_PROFILER )


add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} ${DEPENDENCY_LIST} )

if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
  add_library ( ${LIB_ID}_F ${F90_SOURCE_FILES} )
  target_link_libraries ( ${LIB_ID}_F ${LIB_ID} )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
