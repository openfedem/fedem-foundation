# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFlrLib )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


if ( NOT "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  string ( APPEND CMAKE_CXX_FLAGS " -DFFR_LISTVIEW" )
endif ( NOT "${APPLICATION_ID}" STREQUAL "fedemKernel" )

if ( USE_PROFILER )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_PROFILER" )
endif ( USE_PROFILER )

if ( USE_MEMPOOL )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_MEMPOOL" )
endif ( USE_MEMPOOL )

if ( USE_VERTEXOBJ )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_VERTEX" )
endif ( USE_VERTEXOBJ )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFlrFEResultBuilder FFlrFEResult
                          FFlrFringeCreator FFlrResultResolver
    )
## Pure header files, i.e., header files without a corresponding source file
set ( HEADER_FILE_LIST FapFringeSetup )
## Pure implementation files, i.e., source files without corresponding header
set ( SOURCE_FILE_LIST )


foreach ( FILE ${COMPONENT_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
  list ( APPEND HPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${COMPONENT_FILE_LIST} )

foreach ( FILE ${HEADER_FILE_LIST} )
  list ( APPEND HPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${HEADER_FILE_LIST} )

foreach ( FILE ${SOURCE_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
endforeach ( FILE ${SOURCE_FILE_LIST} )

add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${HPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} FFlVisualization )
