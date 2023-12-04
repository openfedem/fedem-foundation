!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaDeviceFunctionInterface.f90
!> @brief Fortran interface for FiDeviceFunctionFactory methods.
!>
!> @details This file contains a module with Fortran interface definitions
!> for some of the methods of the FiDeviceFunctionFactory class.
!> This is a sub-set of the interface defined in fiDeviceFunctionInterface.f90,
!> containing only the subroutines needed by the FFaFunctionLib module.

!!==============================================================================
!> @brief Fortran interface for FiDeviceFunctionFactory methods.

module FFaDeviceFunctionInterface

  implicit none

  interface

     !> @brief Evaluates a device function at a given point.
     !> @param[in] fileId File handle
     !> @param[in] arg Function argument
     !> @param err Error flag. On input it specifies the integration order
     !> @param[in] channel Channel index for external functions or multi-columns
     !> @param[in] zeroAdj Flag for adjusting first function value to zero
     !> @param[in] vertShift Additional vertical shift of function values
     !> @param[in] scaleFac Scaling factor to apply on function values
     function fidf_getvalue (fileId,arg,err,channel,zeroAdj,vertShift,scaleFac)
       integer , parameter     :: dp = kind(1.0D0)
       real(dp)                :: fidf_getvalue
       integer , intent(in)    :: fileId, zeroAdj, channel
       real(dp), intent(in)    :: arg, vertShift, scaleFac
       integer , intent(inout) :: err
     end function fidf_getvalue

  end interface

end module FFaDeviceFunctionInterface
