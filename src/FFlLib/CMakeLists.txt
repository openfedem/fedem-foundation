# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFlLib )
set ( LIB_ID_LIST FFlFEParts FFlIOAdaptors )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  list ( APPEND LIB_ID_LIST FFlTests )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


if ( USE_PROFILER )
  string ( APPEND CMAKE_CXX_FLAGS " -DFFL_TIMER" )
endif ( USE_PROFILER )

if ( USE_VERTEXOBJ )
  list ( INSERT LIB_ID_LIST 2 FFlVisualization )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_VERTEX" )
endif ( USE_VERTEXOBJ )

if ( USE_VISUALS )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_VISUALS" )
endif ( USE_VISUALS )

if ( USE_MEMPOOL )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_MEMPOOL" )
endif ( USE_MEMPOOL )

string ( APPEND CMAKE_CXX_FLAGS_DEBUG " -DFFL_DEBUG=2" )


# Subfolder handling

foreach ( FOLDER ${LIB_ID_LIST} )
  add_subdirectory ( ${FOLDER} )
endforeach ( FOLDER ${LIB_ID_LIST} )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFlAttributeBase FFlConnectorItems FFlElementBase
                          FFlFEAttributeRefs FFlFEAttributeSpec
                          FFlFEElementTopSpec FFlFENodeRefs FFlFieldBase
                          FFlGroup FFlLinkHandler FFlLoadBase FFlMemPool
                          FFlNamedPartBase FFlPartBase FFlUtils
)
if ( USE_VERTEXOBJ )
  list ( APPEND COMPONENT_FILE_LIST FFlVertex )
endif ( USE_VERTEXOBJ )
if ( USE_VISUALS )
  list ( APPEND COMPONENT_FILE_LIST FFlVisualBase FFlVisualRefs )
endif ( USE_VISUALS )

## Pure header files, i.e., header files without a corresponding source file
set ( HEADER_FILE_LIST FFlFEResultBase FFlField
                       FFlLinkCSMask FFlReference FFlTypeInfoSpec
)

## Pure implementation files, i.e., source files without corresponding header
set ( SOURCE_FILE_LIST )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  set ( SOURCE_FILE_LIST FFlLinkHandler_F )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )

## Fortran 90 source files
set ( F90_FILE_LIST FFlLinkHandlerInterface )


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


set ( DEPENDENCY_LIST FFlFEParts FFaAlgebra FFaDefinitions )

if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  list ( INSERT DEPENDENCY_LIST 0 FFlIOAdaptors )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )

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
  target_link_libraries ( ${LIB_ID}_F ${LIB_ID} FFaCmdLineArg )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
