# SPDX-FileCopyrightText: 2023 SAP SE
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of FEDEM - https://openfedem.org

# Library setup

set ( LIB_ID FFaLib )
set ( LIB_ID_LIST FFaAlgebra FFaCmdLineArg FFaDefinitions
                  FFaOperation FFaOS FFaString
    )
if ( USE_MEMPOOL )
  list ( APPEND LIB_ID_LIST FFaPatterns )
endif ( USE_MEMPOOL )
if ( USE_PROFILER )
  list ( APPEND LIB_ID_LIST FFaProfiler )
endif ( USE_PROFILER )
if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  list ( APPEND LIB_ID_LIST FFaTests )
else ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
  # These libraries are used by the GUI only
  if ( USE_CONNECTORS )
    list ( APPEND LIB_ID_LIST FFaGeometry )
  endif ( USE_CONNECTORS )
  list ( APPEND LIB_ID_LIST FFaContainers FFaDynCalls FFaTypeCheck )
endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" )
set ( UNIT_ID ${DOMAIN_ID}_${PACKAGE_ID}_${LIB_ID} )

if ( USE_PROFILER )
  string ( APPEND CMAKE_Fortran_FLAGS " -DFT_USE_PROFILER" )
elseif ( FTENV_WARNINGS AND LINUX AND NOT USE_INTEL_FORTRAN )
  if ( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0 ) # Seems there is a bug in gcc 5.4
    # causing the compiler to crash with this option, so add only for newer compilers
    string ( APPEND CMAKE_Fortran_FLAGS " -Wno-unused-dummy-argument" )
  endif ( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0 )
endif ( USE_PROFILER )

message ( STATUS "INFORMATION : Processing unit ${UNIT_ID}" )


# Subfolder handling

foreach ( FOLDER ${LIB_ID_LIST} )
  add_subdirectory ( ${FOLDER} )
endforeach ( FOLDER ${LIB_ID_LIST} )


## Fortran 90 source files
set ( F90_FILE_LIST FFaAlgebra/FFaBodyHandlerInterface
                    FFaAlgebra/FFaTensorTransformsInterface
                    FFaCmdLineArg/FFaCmdLineArgInterface
                    FFaProfiler/FFaProfilerInterface
                    FFaOS/FFaFilePathInterface
)


foreach ( FILE ${F90_FILE_LIST} )
  list ( APPEND F90_SOURCE_FILES ${FILE}.f90 )
endforeach ( FILE ${F90_FILE_LIST} )


if ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )

  add_library ( ${LIB_ID}_F ${F90_SOURCE_FILES} )
  target_link_libraries ( ${LIB_ID}_F FFaAlgebra FFaCmdLineArg FFaOS )
  if ( USE_PROFILER )
    target_link_libraries ( ${LIB_ID}_F FFaProfiler )
  endif ( USE_PROFILER )

endif ( "${APPLICATION_ID}" STREQUAL "fedemKernel" AND USE_FORTRAN )
