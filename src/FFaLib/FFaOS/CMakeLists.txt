# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFaOS )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFaDynamicLibraryBase FFaFilePath FFaTag )

## Pure header files, i.e., header files without a corresponding source file
set ( HPP_FILE_LIST FFaExport FFaFortran FFaIO )

## Pure implementation files, i.e., source files without corresponding header
if ( WIN )
  set ( CPP_FILE_LIST FFaMatherrHandler )
else ( WIN )
  set ( CPP_FILE_LIST )
endif ( WIN )

## Special c files
set ( H_FILE_LIST FFaTag_C )
if ( USE_LOWLEVEL_IO )
  set ( C_FILE_LIST FFaIO )
  string ( APPEND CMAKE_CXX_FLAGS " -DFT_USE_LOWLEVEL_IO" )
else ( USE_LOWLEVEL_IO )
  set ( C_FILE_LIST )
endif ( USE_LOWLEVEL_IO )


foreach ( FILE ${COMPONENT_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
  list ( APPEND HPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${COMPONENT_FILE_LIST} )

foreach ( FILE ${HPP_FILE_LIST} )
  list ( APPEND HPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${HPP_FILE_LIST} )

foreach ( FILE ${CPP_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
endforeach ( FILE ${CPP_FILE_LIST} )

foreach ( FILE ${H_FILE_LIST} )
  list ( APPEND HPP_HEADER_FILES ${FILE}.h )
endforeach ( FILE ${H_FILE_LIST} )

foreach ( FILE ${C_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.c )
endforeach ( FILE ${C_FILE_LIST} )


add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${HPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} FFaDefinitions )
if ( UNIX )
  find_library ( DL_LIBRARIES dl /lib64/ )
  target_link_libraries ( ${LIB_ID} ${DL_LIBRARIES} )
endif ( UNIX )
