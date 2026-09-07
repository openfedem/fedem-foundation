!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFpFatigueInterface.f90
!> @brief Fortran interface for the fatigue calculation methods.
!> @details This file contains a module with Fortran interface definitions for
!> the methods of the damage and fatigue classes. See the file FFpFatigue_F.C
!> for the implementation of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for fatigue calculation methods.

module FFpFatigueInterface

  implicit none

  interface

     !> @brief Initializes the fatigue calculation module.
     !> @param[out] ierr Error indicator
     subroutine ffp_initfatigue (ierr)
       integer, intent(out) :: ierr
     end subroutine ffp_initfatigue

     !> @brief Adds a response point to the specified time history.
     !> @param[in] handle Time history handle
     !> @param[in] time Actual time
     !> @param[in] value Response value associated with @a time
     subroutine ffp_addpoint (handle, time, value)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(inout) :: handle
       real(dp), intent(in)    :: time, value
     end subroutine ffp_addpoint

     !> @brief Releases all data associated with the sprcified time history.
     !> @param[in] handle Time history handle
     subroutine ffp_releasedata (handle)
       integer, intent(in) :: handle
     end subroutine ffp_releasedata

     !> @brief Calculates the accumulated damage for the specified time history.
     !> @param[in] handle Time history handle
     !> @param[in] snCurve S-N curve identifiers
     !> @param[in] gateValue Ignore response chnages smaller than this value
     !> @param[out] damage The calculated damage
     !> @param[out] ierr Error indicator
     subroutine ffp_calcdamage (handle, snCurve, gateValue, damage, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: handle, snCurve(2)
       real(dp), intent(in)  :: gateValue
       real(dp), intent(out) :: damage
       integer , intent(out) :: ierr
     end subroutine ffp_calcdamage

     !> @brief Returns the accumulated damage for the specified time history.
     !> @param[in] handle Time history handle
     !> @param[in] gateValue Ignore response chnages smaller than this value
     !> @param[in] curveData Data defining the S-N curve to use
     !> @param[in] printCycles If .true., print the cycles to file
     function ffp_getdamage (handle, gateValue, curveData, printCycles)
       integer , parameter  :: dp = kind(1.0D0)
       integer , intent(in) :: handle
       real(dp), intent(in) :: gateValue, curveData(4)
       integer , intent(in), optional :: printCycles
       real(dp) :: ffp_getdamage
     end function ffp_getdamage

     !> @brief Returns the number of full cycles for the specified time history.
     !> @param[in] handle Time history handle
     !> @param[in] low Lower bound of the range to consider
     !> @param[in] high Upper bound of the range to consider
     function ffp_getnumcycles (handle, low, high)
       integer , parameter  :: dp = kind(1.0D0)
       integer , intent(in) :: handle
       real(dp), intent(in) :: low, high
       integer :: ffp_getnumcycles
     end function ffp_getnumcycles

  end interface

end module FFpFatigueInterface
