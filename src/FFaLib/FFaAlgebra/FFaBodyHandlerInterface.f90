!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

module FFaBodyHandlerInterface

  implicit none

  interface

     function ffa_body (fileName)
       integer                      :: ffa_body
       character(len=*), intent(in) :: fileName
     end function ffa_body

     subroutine ffa_get_nofaces (bodyIndex,nface)
       integer , intent(in)  :: bodyIndex
       integer , intent(out) :: nface
     end subroutine ffa_get_nofaces

     subroutine ffa_get_face (bodyIndex, fIndex, coords, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex, fIndex
       real(dp), intent(out) :: coords(3,4)
       integer , intent(out) :: ierr
     end subroutine ffa_get_face

     subroutine ffa_partial_volume (bodyIndex, normal, z0, &
          &                         Vb, As, C0b, C0s, ierr)

       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex
       real(dp), intent(in)  :: normal(3), z0
       real(dp), intent(out) :: Vb, As, C0b(3), C0s(3)
       integer , intent(out) :: ierr
     end subroutine ffa_partial_volume

     subroutine ffa_total_volume (bodyIndex, V, C0, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex
       real(dp), intent(out) :: V, C0(3)
       integer , intent(out) :: ierr
     end subroutine ffa_total_volume

     subroutine ffa_save_intersection (bodyIndex, CS, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex
       real(dp), intent(in)  :: CS(3,4)
       integer , intent(out) :: ierr
     end subroutine ffa_save_intersection

     subroutine ffa_inc_area (bodyIndex, normal, CS, dA, C0, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex
       real(dp), intent(in)  :: normal(3), CS(3,4)
       real(dp), intent(out) :: dA, C0(3)
       integer , intent(out) :: ierr
     end subroutine ffa_inc_area

     subroutine ffa_erase_bodies ()
     end subroutine ffa_erase_bodies

  end interface

contains

  subroutine ffa_initbody (fileName,bodyIndex)
    character(len=*), intent(in)  :: fileName
    integer         , intent(out) :: bodyIndex
    bodyIndex = ffa_body(fileName)
  end subroutine ffa_initbody

end module FFaBodyHandlerInterface
