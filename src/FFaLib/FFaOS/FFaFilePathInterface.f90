!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaFilePathInterface.f90
!> @brief Fortran interface for FFaFilePath methods.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!> @brief Fortran interface for FFaFilePath methods.

module FFaFilePathInterface

  implicit none

  interface

     !> @brief Converts @a thePath from UNIX to Windows syntax and vice versa.
     subroutine ffa_checkpath (thePath)
       character*(*), intent(inout) :: thePath
     end subroutine ffa_checkpath

     !> @brief Returns the file name without the path and extension.
     !> @param[in] thePath The full pathname
     !> @param[out] baseName The file name without path and extension
     subroutine ffa_getbasename (thePath,baseName)
       character*(*), intent(in)  :: thePath
       character*(*), intent(out) :: baseName
     end subroutine ffa_getbasename

  end interface

end module FFaFilePathInterface
