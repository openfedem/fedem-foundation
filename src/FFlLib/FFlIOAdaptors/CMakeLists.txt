# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFlIOAdaptors )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )

if ( USE_VKI )
  find_package ( VKIAPI )
endif ( USE_VKI )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFlFedemReader FFlFedemWriter FFlNastranReader
                          FFlOldFLMReader FFlReaderBase FFlReaders FFlVdmReader
                          FFlVTFWriter FFlSesamReader FFlAllIOAdaptors
                          FFlCrossSection
)

## Pure header files, i.e., header files without a corresponding source file
set ( HEADER_FILE_LIST FFlWriterBase )

## Pure implementation files, i.e., source files without corresponding header
set ( SOURCE_FILE_LIST FFlNastranProcessor FFlNastranResolver )


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


add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} FFlLib Admin ${VKI_LIBRARY} ${VTF_LIBRARY} )
