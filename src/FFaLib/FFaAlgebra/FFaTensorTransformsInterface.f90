!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaTensorTransformsInterface.f90
!> @brief Fortran interface for FFaTensorTransforms methods.
!> @details This file contains a module with Fortran interface definitions
!> for methods of the FFaTensorTransforms class.
!> See the file FFaTensorTransforms_F.C
!> for the implementation of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FFaTensorTransforms methods.

module FFaTensorTransformsInterface

  implicit none

  interface

     !> @brief Returns the von Mises of given stress/strain tensor.
     !> @param[in] N Number of space dimensions (1, 2 or 3)
     !> @param[in] S Cartesian stress/strain tensor components
     !> @return The von Mises stress/strain value
     function vonmises (N,S)
       integer , parameter  :: dp = kind(1.0D0)
       real(dp)             :: vonmises
       integer , intent(in) :: N
       real(dp), intent(in) :: S(*)
     end function vonmises

     !> @brief Calculates the principal values of given stress/strain tensor.
     !> @param[in] N Number of space dimensions (1, 2 or 3)
     !> @param[in] S Cartesian stress/strain tensor components
     !> @param[out] Pval The principal stress/strain component
     subroutine princval (N,S,Pval)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: N
       real(dp), intent(in)  :: S(*)
       real(dp), intent(out) :: Pval(*)
     end subroutine princval

     !> @brief Calculates the maximum shear value of given stress/strain tensor.
     !> @param[in] N Number of space dimensions (1, 2 or 3)
     !> @param[in] S Cartesian stress/strain tensor components
     !> @param[out] maxShear The maximum shear stress/strain value
     subroutine maxshearvalue (N,Pval,maxShear)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: N
       real(dp), intent(in)  :: Pval(*)
       real(dp), intent(out) :: maxShear
     end subroutine maxshearvalue

     !> @brief Calculates the maximum shear of given stress/strain tensor.
     !> @param[in] N Number of space dimensions (1, 2 or 3)
     !> @param[in] S Cartesian stress/strain tensor components
     !> @param[out] Pval The maximum shear stress/strain value
     !> @param[out] Pdir The direction of the maximum shear value
     subroutine maxshear (N,Pval,Pdir,Shear,Sdir)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: N
       real(dp), intent(in)  :: Pval(*), Pdir(*)
       real(dp), intent(out) :: Shear, Sdir(*)
     end subroutine maxshear

     !> @brief Congruence transformation of 2D and 3D symmetric tensors.
     !> @param[in] N Number of space dimensions (2 or 3)
     !> @param S Cartesian stress/strain tensor components
     !> @param[in] Tmat @a N&times;N transformation matrix
     subroutine tratensor (N,S,Tmat)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: N
       real(dp), intent(inout) :: S(*)
       real(dp), intent(in)    :: Tmat(*)
     end subroutine tratensor

     !> @brief Inertia tensor transformation based on the parallel-axis theorem.
     !> @param[in] N Number of space dimensions (should be 3)
     !> @param I The 6 components of the symmetric 3D inertia tensor
     !> @param[in] Xvec Offset between the intertia origin and center of mass
     !> @param[in] Mass The mass asociated with the inertia tensor @a I
     subroutine trainertia (N,I,Xvec,Mass)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: N
       real(dp), intent(inout) :: I(*)
       real(dp), intent(in)    :: XVec(*), Mass
     end subroutine trainertia

  end interface

end module FFaTensorTransformsInterface
