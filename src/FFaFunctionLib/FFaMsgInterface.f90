!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaMsgInterface.f90
!> @brief Fortran interface for FFaMsg methods.
!>
!> @details This file contains a module with Fortran interface definitions
!> for some of the methods of the FFaMsg class.
!> See the file FFaMsg_F.C for the implementation
!> of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FFaMsg methods.

module FFaMsgInterface

  implicit none

  interface

     !> @brief Prints the @a text message to the Output List view and/or file.
     subroutine ffamsg_list (text,ival)
       character(len=*), intent(in) :: text
       integer,optional, intent(in) :: ival
     end subroutine ffamsg_list

  end interface

end module FFaMsgInterface
