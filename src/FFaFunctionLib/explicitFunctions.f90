!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file explicitFunctions.f90
!> @brief Global functions callable from C++ code.
!>
!> @details This file contains global function wrappers of subroutines from the
!> explicitfunctionsmodule, such that they can be invoked from C++ or python.

!!==============================================================================
!> @brief Evaluates an explicit function.
!> @param[in] baseID Base Id of the explicit function to evaluate
!> @param[in] intParams Integer function parameters (constant)
!> @param[in] realParams Double precision function parameters (constant)
!> @param[in] nip Number of integer parameters
!> @param[in] nrp Number of double precision parameters
!> @param[in] xArg Function argument value
!> @param[out] ierr Error flag
!> @return The function value

function getFunctionValue (baseID, intParams, realParams, nip, nrp, xArg, ierr)

  use ExplicitFunctionsModule, only : funcValue

  implicit none

  integer , parameter   :: dp = kind(1.0D0) ! 8-byte real (double)
  integer , intent(in)  :: nip, nrp, baseID, intParams(nip)
  real(dp), intent(in)  :: xArg, realParams(nrp)
  integer , intent(out) :: ierr
  real(dp)              :: getFunctionValue

  !! --- Logic section ---

  ierr = 0
  call funcValue (baseID,intParams,realParams,xArg,getFunctionValue,ierr)

end function getFunctionValue


!!==============================================================================
!> @brief Evaluates the derivative of an explicit function.
!> @param[in] baseID Base Id of the explicit function to evaluate
!> @param[in] intParams Integer function parameters (constant)
!> @param[in] realParams Double precision function parameters (constant)
!> @param[in] nip Number of integer parameters
!> @param[in] nrp Number of double precision parameters
!> @param[in] xArg Function argument value
!> @param[out] ierr Error flag
!> @return The function derivative

function getFunctionDeriv (baseID, intParams, realParams, nip, nrp, xArg, ierr)

  use ExplicitFunctionsModule, only : funcDerivative

  implicit none

  integer , parameter   :: dp = kind(1.0D0) ! 8-byte real (double)
  integer , intent(in)  :: nip, nrp, baseID, intParams(nip)
  real(dp), intent(in)  :: xArg, realParams(nrp)
  integer , intent(out) :: ierr
  real(dp)              :: getFunctionDeriv

  !! --- Logic section ---

  ierr = 0
  call funcDerivative (baseID,1,intParams,realParams,xArg,getFunctionDeriv,ierr)

end function getFunctionDeriv


!!==============================================================================
!> @brief Returns the function type Id (enum) of the specified @a functionName.
function getFunctionTypeID (functionName)

  use ExplicitFunctionsModule, only : maxFunc_p, funcType_p

  implicit none

  character(len=*), intent(in) :: functionName
  integer                      :: getFunctionTypeID

  !! Local variables
  integer :: i

  !! --- Logic section ---

  do i = 1, maxFunc_p
     if (trim(funcType_p(i)) == functionName) then
        getFunctionTypeID = i
        return
     end if
  end do

  getFunctionTypeID = -1

end function getFunctionTypeID
