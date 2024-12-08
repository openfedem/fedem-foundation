# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFlFEParts )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFlAllFEParts FFlBeams FFlBUSH
                          FFlCMASS FFlCurvedFace FFlLoads FFlNode
                          FFlPBEAMECCENT FFlPBEAMPIN
                          FFlPBEAMSECTION FFlPBUSHCOEFF FFlPBUSHECCENT
                          FFlPCOMP FFlPCOORDSYS FFlPEFFLENGTH
                          FFlPMASS FFlPMAT FFlPNSM
                          FFlPORIENT FFlPRBAR FFlPRGD FFlPSPRING
                          FFlPTHICK FFlPWAVGM
                          FFlRBAR FFlRGD FFlShells FFlSolids
                          FFlSPRING FFlWAVGM
)
if ( USE_STRAINCOAT )
  list ( APPEND COMPONENT_FILE_LIST FFlPFATIGUE FFlPHEIGHT FFlPSTRC
                                    FFlPTHICKREF FFlSTRCoat )
endif ( USE_STRAINCOAT )
if ( USE_VISUALS )
  list ( APPEND COMPONENT_FILE_LIST FFlVAppearance FFlVDetail )
endif ( USE_VISUALS )


foreach ( FILE ${COMPONENT_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
  list ( APPEND CPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${COMPONENT_FILE_LIST} )


add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} FFlLib )
