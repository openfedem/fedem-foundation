# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Build setup

set ( LIB_ID_LIST Admin FFaLib FFlLib FFrLib FFpLib
                  FFaMathExpr FFaFunctionLib
                  FiDeviceFunctions FiUserElmPlugin )
if ( USE_VERTEXOBJ )
  list ( INSERT LIB_ID_LIST 3 FFlrLib )
endif ( USE_VERTEXOBJ )

set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )

include_directories ( "${CMAKE_CURRENT_SOURCE_DIR}" )

if ( USE_FORTRAN )
  set ( CMAKE_Fortran_MODULE_DIRECTORY "${PROJECT_BINARY_DIR}/f90_modules" )
endif ( USE_FORTRAN )

foreach ( FOLDER ${LIB_ID_LIST} )
  add_subdirectory ( ${FOLDER} )
endforeach ( FOLDER ${LIB_ID_LIST} )
