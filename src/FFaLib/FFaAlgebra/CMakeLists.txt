# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFaAlgebra )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFa3DLocation FFa3PArc FFaAlgebra FFaBody
                          FFaCheckSum FFaMath FFaMat33 FFaMat34 FFaVec3
                          FFaTensor1 FFaTensor2 FFaTensor3
                          FFaTensorTransforms
                          FFaUnitCalculator
                          FFaVolume
)

## Pure implementation files, i.e., source files without corresponding header
set ( SOURCE_FILE_LIST FFaBodyParser )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
  list ( APPEND SOURCE_FILE_LIST FFaBodyHandler_F FFaTensorTransforms_F )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )


foreach ( FILE ${COMPONENT_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
  list ( APPEND CPP_HEADER_FILES ${FILE}.H )
endforeach ( FILE ${COMPONENT_FILE_LIST} )

foreach ( FILE ${SOURCE_FILE_LIST} )
  list ( APPEND CPP_SOURCE_FILES ${FILE}.C )
endforeach ( FILE ${SOURCE_FILE_LIST} )


# Dependencies and external libraries

set ( DEPENDENCY_LIST FFaString FFaDefinitions )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  list ( APPEND DEPENDENCY_LIST FFaOS )
  # Include Lapack binding only when building the fedem kernel
  if ( USE_INTEL_FORTRAN )
    set ( BLA_VENDOR Intel10_64lp )
  endif ( USE_INTEL_FORTRAN )
  find_package ( LAPACK )
  if ( LAPACK_FOUND )
    message ( STATUS "Using LAPACK::DSYEV for calculation of principal values" )
    string ( APPEND CMAKE_CXX_FLAGS " -DFT_HAS_LAPACK" )
    list ( APPEND DEPENDENCY_LIST ${LAPACK_LIBRARIES} )
  endif ( LAPACK_FOUND )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )

add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
target_link_libraries ( ${LIB_ID} ${DEPENDENCY_LIST} )
