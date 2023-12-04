!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FiUserElmInterface.f90
!> @brief Fortran interface for FiUserElmPlugin methods.
!> @details This file contains a module with Fortran interface definitions for
!> the methods of the FiUserElmPlugin class. See the file FiUserElmPlugin_F.C
!> for the implementation of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FiUserElmPlugin methods.

module FiUserElmInterface

  implicit none

  interface

     !!=========================================================================
     !> @brief Loads the user-defined element plugin library into memory.
     !> @param[in] gdata Global parameters that applies to all element instances
     !> @param[out] sign Description of the loaded library
     !> @param[out] ierr Error flag
     subroutine Fi_UDE_Init (gdata,sign,ierr)
       integer         , parameter   :: dp = kind(1.0D0)
       real(dp)        , intent(in)  :: gdata
       character(len=*), intent(out) :: sign
       integer         , intent(out) :: ierr
     end subroutine Fi_UDE_Init

     !!=========================================================================
     !> @brief Returns the length of the work arrays needed by an element.
     !> @param[in] eId   Unique id identifying this element instance
     !> @param[in] eType Unique id identifying the element type
     !> @param[in] nenod Number of nodes in the element
     !> @param[in] nedof Number of degrees of freedom in the element
     !> @param[out] niwork Required size of the integer work area
     !> @param[out] nrwork Required size of the double precision work area
     subroutine Fi_UDE0 (eId,eType,nenod,nedof,niwork,nrwork)
       integer , intent(in)  :: eId, eType, nenod, nedof
       integer , intent(out) :: niwork, nrwork
     end subroutine Fi_UDE0

     !!=========================================================================
     !> @brief Initializes the state-independent part of the ework areas.
     !> @param[in] eId   Unique id identifying this element instance
     !> @param[in] eType Unique id identifying the element type
     !> @param[in] nenod Number of nodes in the element
     !> @param[in] nedof Number of degrees of freedom in the element
     !> @param[in] X     Global coordinas of the element nodes
     !> @param[in] T     Local coordinate systems of the element nodes
     !> @param     iwork Integer work area for this element
     !> @param     rwork Double precision work area for this element
     !> @param[out] ierr Error flag
     subroutine Fi_UDE1 (eId,eType,nenod,nedof,X,T,iwork,rwork,ierr)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: eId, eType, nenod, nedof
       real(dp), intent(in)    :: X, T
       integer , intent(inout) :: iwork
       real(dp), intent(inout) :: rwork
       integer , intent(out)   :: ierr
     end subroutine Fi_UDE1

     !!=========================================================================
     !> @brief Updates the state of a given user-defined element.
     !> @param[in] eId   Unique id identifying this element instance
     !> @param[in] eType Unique id identifying the element type
     !> @param[in] nenod Number of nodes in the element
     !> @param[in] nedof Number of degrees of freedom in the element
     !> @param[in] X Global nodal coordinates of the element
     !> @param[in] T Local nodal coordinate systems of the element
     !> @param[in] v Global nodal velocities of the element
     !> @param[in] a Global nodal accelerations of the element
     !> @param     iwork Integer work area for this element
     !> @param     rwork Real work area for this element
     !> @param[out] K  Tangent stiffness matrix
     !> @param[out] C  Damping matrix
     !> @param[out] M  Mass matrix
     !> @param[out] Fs Internal elastic forces
     !> @param[out] Fd Damping forces
     !> @param[out] Fi Intertia forces
     !> @param[out] Q  External forces
     !> @param[in] time  Current time
     !> @param[in] dt    Time step size
     !> @param[in] istep Time step number
     !> @param[in] iter  Iteration number
     !> @param[out] ierr Error flag
     subroutine Fi_UDE2 (eId,eType,nenod,nedof,X,T,v,a,iwork,rwork, &
          &              K,C,M,Fs,Fd,Fi,Q,time,dt,istep,iter,ierr)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: eId, eType, nenod, nedof, istep, iter
       real(dp), intent(in)    :: X, T, v, a, time, dt
       integer , intent(inout) :: iwork
       real(dp), intent(inout) :: rwork
       real(dp), intent(out)   :: K, C, M, Fs, Fd, Fi, Q
       integer , intent(out)   :: ierr
     end subroutine Fi_UDE2

     !!=========================================================================
     !> @brief Calculates the local origin of a user-defined element.
     !> @param[in] eId   Unique id identifying this element instance
     !> @param[in] eType Unique id identifying the element type
     !> @param[in] nenod Number of element nodes
     !> @param[in] X     Global nodal coordinates of the element
     !> @param[in] T     Local nodal coordinate systems of the element
     !> @param     iwork Integer work area for this element
     !> @param[in] rwork Real work area for this element
     !> @param[out] Tlg  Local-to-global transformation matrix for the element
     !> @param[out] ierr Error flag
     subroutine Fi_UDE3 (eId,eType,nenod,X,T,iwork,rwork,Tlg,ierr)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: eId, eType, nenod
       real(dp), intent(in)    :: X, T
       integer , intent(inout) :: iwork
       real(dp), intent(inout) :: rwork
       real(dp), intent(out)   :: Tlg
       integer , intent(out)   :: ierr
     end subroutine Fi_UDE3

     !!=========================================================================
     !> @brief Returns the name of a result quantity of a user-defined element.
     !> @param[in] eId   Unique id identifying this element instance
     !> @param[in] eType Unique id identifying the element type
     !> @param[in] idx   Result quantity index
     !> @param[in] iwork Integer work area for this element
     !> @param[in] rwork Real work area for this element
     !> @param[out] name Name of result quantity
     !> @param[out] nvar Total number of result quantities for this element
     subroutine Fi_UDE4 (eId,eType,idx,iwork,rwork,name,nvar)
       integer         , parameter   :: dp = kind(1.0D0)
       integer         , intent(in)  :: eId, eType, idx
       integer         , intent(in)  :: iwork
       real(dp)        , intent(in)  :: rwork
       character(len=*), intent(out) :: name
       integer         , intent(out) :: nvar
     end subroutine Fi_UDE4

     !!=========================================================================
     !> @brief Returns a result quantity value of a user-defined element.
     !> @param[in] eId   Unique id identifying this element instance
     !> @param[in] eType Unique id identifying the element type
     !> @param[in] idx   Result quantity index
     !> @param[in] iwork Integer work area for this element
     !> @param[in] rwork Real work area for this element
     !> @param[out] value The result quantity value
     !> @param[out] nvar Total number of result quantities for this element
     subroutine Fi_UDE5 (eId,eType,idx,iwork,rwork,value,nvar)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: eId, eType, idx
       integer , intent(in)  :: iwork
       real(dp), intent(in)  :: rwork
       real(dp), intent(out) :: value
       integer , intent(out) :: nvar
     end subroutine Fi_UDE5

     !!=========================================================================
     !> @brief Returns total mass of a user-defined element.
     !> @param[in] eId   Unique id identifying this element instance
     !> @param[in] eType Unique id identifying the element type
     !> @param[in] nenod Number of element nodes
     !> @param[in] X     Global nodal coordinates of the element
     !> @param[in] iwork Integer work area for this element
     !> @param[in] rwork Real work area for this element
     !> @param[out] mass Total mass of the element
     !> @param[out] ierr Error flag
     subroutine Fi_UDE6 (eId,eType,nenod,X,iwork,rwork,mass,ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: eId, eType, nenod
       real(dp), intent(in)  :: X
       integer , intent(in)  :: iwork
       real(dp), intent(in)  :: rwork
       real(dp), intent(out) :: mass
       integer , intent(out) :: ierr
     end subroutine Fi_UDE6

  end interface

end module FiUserElmInterface
