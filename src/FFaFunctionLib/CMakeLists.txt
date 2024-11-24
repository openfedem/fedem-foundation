# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFaFunctionLib )
set ( LIB_ID_LIST )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  set ( LIB_ID_LIST FFaFunctionDLL )
  if ( BUILD_TESTS )
    list ( APPEND LIB_ID_LIST FFaFunctionTests )
  endif ( BUILD_TESTS )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


# Subfolder handling

foreach ( FOLDER ${LIB_ID_LIST} )
  add_subdirectory ( ${FOLDER} )
endforeach ( FOLDER ${LIB_ID_LIST} )


## Files with header and source with same name
set ( COMPONENT_FILE_LIST FFaFunctionManager FFaFunctionProperties FFaUserFuncPlugin )
## Pure header files, i.e., header files without a corresponding source file
set ( HEADER_FILE_LIST )
## Pure implementation files, i.e., source files without corresponding header
set ( SOURCE_FILE_LIST FFaUserFuncPlugin_F )

## Fortran 90 source files
set ( F90_FILE_LIST FFaMsgInterface
     ../FFaMathExpr/FFaMathExprInterface
                    FFaDeviceFunctionInterface
                    FFaUserFuncInterface
                    explicitFunctionsModule
                    waveFunctionsModule
                    explicitFunctions
                    waveFunctions
    )


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

option ( USE_OPENMP "Build with OpenMP-parallelisation of wave functions" false )
option ( USE_CWAVE "Use plugin for nonlinear wave functions (Univ. of Southampton)" false )
mark_as_advanced ( USE_OPENMP USE_CWAVE )

set ( DEPENDENCY_LIST FFaMathExpr FiDeviceFunctions FFaAlgebra )
if ( USE_FORTRAN )
  list ( INSERT DEPENDENCY_LIST 0 ${LIB_ID}_F90 )
  if ( USE_OPENMP AND WIN )
    set ( CMAKE_Fortran_FLAGS_RELEASE "/Qopenmp" )
  endif ( USE_OPENMP AND WIN )
  if ( USE_CWAVE )
    find_package ( Cwave )
    if ( CWave_LIBRARY )
      string ( APPEND CMAKE_Fortran_FLAGS " -DFT_HAS_CWAVE" )
      list ( INSERT DEPENDENCY_LIST 1 ${CWave_LIBRARY} )
    endif ( CWave_LIBRARY )
  endif ( USE_CWAVE )
else ( USE_FORTRAN )
  string ( APPEND CMAKE_CXX_FLAGS " -D_NO_FORTRAN" )
endif ( USE_FORTRAN )


# NOTE: Fortran and C-code separated in order to build in Visual Studio.
# The order of adding and linking libraries has implications for libraries
# in other parts of the code. In particular vpm-libraries. RHR 1/9-2011.
add_library ( ${LIB_ID} ${CPP_SOURCE_FILES} ${CPP_HEADER_FILES} )
if ( USE_FORTRAN )
  add_library ( ${LIB_ID}_F90 ${F90_SOURCE_FILES} )
endif ( USE_FORTRAN )
target_link_libraries ( ${LIB_ID} ${DEPENDENCY_LIST} )
if ( USE_FORTRAN )
  target_link_libraries ( ${LIB_ID}_F90 ${LIB_ID} )
endif ( USE_FORTRAN )
