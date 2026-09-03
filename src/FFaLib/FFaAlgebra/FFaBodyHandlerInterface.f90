!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaBodyHandlerInterface.f90
!> @brief Fortran interface for FFaBodyHandler methods.
!> @details This file contains a module with Fortran interface definitions
!> for methods of the FFaBodyHandler class.
!> See the file FFaBodyHandler_F.C
!> for the implementation of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FFaBodyHandler methods.

module FFaBodyHandlerInterface

  implicit none

  interface

     !> @brief Creates a new body by reading the specified CAD-file
     !> @param[in] fileName Name of the CAD-file to read
     !> @return bodyIndex Handle of the created body (negative on error)
     function ffa_body (fileName)
       integer                      :: ffa_body
       character(len=*), intent(in) :: fileName
     end function ffa_body

     !> @brief Calculates the number of faces for the specified body.
     !> @param[in] bodyIndex Handle of the body to calculate for
     !> @param[out] nface Number of faces in the body
     subroutine ffa_get_nofaces (bodyIndex,nface)
       integer , intent(in)  :: bodyIndex
       integer , intent(out) :: nface
     end subroutine ffa_get_nofaces

     !> @brief Finds the vertex coordinates of a specified body face.
     !> @param[in] bodyIndex Handle of the body to calculate for
     !> @param[out] fIndx Index of the face to consider
     !> @param[out] coords Global coordinates of the face vertices
     !> @param[out] ierr Error indicator
     subroutine ffa_get_face (bodyIndex, fIndex, coords, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex, fIndex
       real(dp), intent(out) :: coords(3,4)
       integer , intent(out) :: ierr
     end subroutine ffa_get_face

     !> @brief Calculates the partial volume of a body under a specified plane.
     !> @param[in] bodyIndex Handle of the body to calculate for
     !> @param[in] normal Normal vector of the intersecting plane
     !> @param[in] z0 Position of the intersection plane along the @a normal
     !> @param[out] Vb Volume of the portion of the body below the plane
     !> @param[out] As Area of the intersection between the body and the plane
     !> @param[out] C0b Centroid of the volume portion below the plane
     !> @param[out] C0s Centroid of the intersection area
     !> @param[out] ierr Error indicator
     subroutine ffa_partial_volume (bodyIndex, normal, z0, &
          &                         Vb, As, C0b, C0s, ierr)

       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex
       real(dp), intent(in)  :: normal(3), z0
       real(dp), intent(out) :: Vb, As, C0b(3), C0s(3)
       integer , intent(out) :: ierr
     end subroutine ffa_partial_volume

     !> @brief Calculates the total volume of a body.
     !> @param[in] bodyIndex Handle of the body to calculate for
     !> @param[out] V Volume of the entire body
     !> @param[out] C0 Centroid of the body
     !> @param[out] ierr Error indicator
     subroutine ffa_total_volume (bodyIndex, V, C0, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex
       real(dp), intent(out) :: V, C0(3)
       integer , intent(out) :: ierr
     end subroutine ffa_total_volume

     !> @brief Stores internally the vertices of current intersection surface.
     !> @param[in] bodyIndex Handle of the body to calculate for
     !> @param[in] CS Local coordinate system for the intersection surface
     !> @param[out] ierr Error indicator
     subroutine ffa_save_intersection (bodyIndex, CS, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex
       real(dp), intent(in)  :: CS(3,4)
       integer , intent(out) :: ierr
     end subroutine ffa_save_intersection

     !> @brief Calculates the increment in the intersection area.
     !> @param[in] bodyIndex Handle of the body to calculate for
     !> @param[in] normal Normal vector of the intersection plane
     !> @param[in] CS Local coordinate system for the intersection surface
     !> @param[out] dA Area increment
     !> @param[out] C0 Centroid of the area increment
     !> @param[out] ierr Error indicator
     subroutine ffa_inc_area (bodyIndex, normal, CS, dA, C0, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: bodyIndex
       real(dp), intent(in)  :: normal(3), CS(3,4)
       real(dp), intent(out) :: dA, C0(3)
       integer , intent(out) :: ierr
     end subroutine ffa_inc_area

     !> @brief Deletes all bodies in the internal cache.
     subroutine ffa_erase_bodies ()
     end subroutine ffa_erase_bodies

  end interface

contains

  !!============================================================================
  !> @brief Creates a new body by reading the specified CAD-file
  !> @param[in] fileName Name of the CAD-file to read
  !> @param[out] bodyIndex Handle of the created body (negative on error)
  subroutine ffa_initbody (fileName,bodyIndex)
    character(len=*), intent(in)  :: fileName
    integer         , intent(out) :: bodyIndex
    bodyIndex = ffa_body(fileName)
  end subroutine ffa_initbody

end module FFaBodyHandlerInterface
