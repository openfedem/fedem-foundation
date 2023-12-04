!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFrExtractorInterface.f90
!> @brief Fortran interface for FFrExtractor methods.
!>
!> @details This file contains a module with Fortran interface definitions
!> for some methods of the FFrExtractor class. See the file FFrExtractor_F.C
!> for the implementation of the wrapper functions of this interface.
!>
!> @author Knut Morten Okstad
!> @date 9 Oct 2000

!!==============================================================================
!> @brief Module with some kind parameters.

module PointerKindModule
  implicit none
  integer, parameter :: i8 = selected_int_kind(18)  !< 8-byte integer
  integer, parameter :: dp = kind(1.0D0)            !< 8-byte real (double)
#if defined(win64) || defined(linux64)
  integer, parameter :: ptr = i8                    !< 8-byte integer
#else
  integer, parameter :: ptr = selected_int_kind(9)  !< 4-byte integer
#endif
end module PointerKindModule


!!==============================================================================
!> @brief Fortran interface for FFrExtractor methods.

module FFrExtractorInterface

  implicit none

  interface

     !> @brief Opens the results database and reads file headers.
     subroutine ffr_init (fname,ierr)
       character, intent(in)  :: fname*(*) !< Command-line option for frs-files
       integer  , intent(out) :: ierr      !< Error flag
     end subroutine ffr_init

     !> @brief Releases the FFrExtractor object.
     subroutine ffr_done
     end subroutine ffr_done

     !> @brief Finds file position of the specified result variable.
     subroutine ffr_findptr (pname, otype, baseid, varPtr)
       use PointerKindModule, only : ptr
       character   , intent(in)  :: pname*(*) !< Variable description path
       character   , intent(in)  :: otype*(*) !< Object group type name
       integer     , intent(in)  :: baseid    !< Base id of the object to search
       integer(ptr), intent(out) :: varPtr    !< C-pointer to result quantity
     end subroutine ffr_findptr

     !> @brief Positions the results file(s) for the specified time step.
     subroutine ffr_setposition (atime, btime, istep)
       use PointerKindModule, only : dp, i8
       real(dp)   , intent(in)  :: atime !< The requested time
       real(dp)   , intent(out) :: btime !< Found time closest to atime
       integer(i8), intent(out) :: istep !< Found time step index
     end subroutine ffr_setposition

     !> @brief Positions the results file(s) for the next time step.
     subroutine ffr_increment (btime, istep)
       use PointerKindModule, only : dp, i8
       real(dp)   , intent(out) :: btime !< The time of next time step
       integer(i8), intent(out) :: istep !< Found time step index
     end subroutine ffr_increment

     !> @brief Reads the contents of the variable or item group.
     subroutine ffr_getdata (data, nword, varPtr, ierr)
       use PointerKindModule, only : dp, ptr
       real(dp)    , intent(out) :: data   !< Array of results data
       integer     , intent(in)  :: nword  !< Length of the results array
       integer(ptr), intent(in)  :: varPtr !< C-pointer to result quantity
       integer     , intent(out) :: ierr   !< Error flag
     end subroutine ffr_getdata

  end interface


  !> @brief Locates results data at current time step for specified variable.
  interface ffr_findData

     !> @brief Locates results data at current time step for integer variables.
     subroutine ffr_intdata (data, nword, pname, otype, baseid, ierr)
       integer  , intent(out) :: data      !< Array of results data
       integer  , intent(in)  :: nword     !< Length of the results array
       character, intent(in)  :: pname*(*) !< Variable description path
       character, intent(in)  :: otype*(*) !< Object group type name
       integer  , intent(in)  :: baseid    !< Base id of the object to search
       integer  , intent(out) :: ierr      !< Error flag
     end subroutine ffr_intdata

     !> @brief Locates results data at current time step for real variables.
     subroutine ffr_realdata (data, nword, pname, otype, baseid, ierr)
       use PointerKindModule, only : dp
       real(dp) , intent(out) :: data      !< Array of results data
       integer  , intent(in)  :: nword     !< Length of the results array
       character, intent(in)  :: pname*(*) !< Variable description path
       character, intent(in)  :: otype*(*) !< Object group type name
       integer  , intent(in)  :: baseid    !< Base id of the object to search
       integer  , intent(out) :: ierr      !< Error flag
     end subroutine ffr_realdata

  end interface


contains

  !!============================================================================
  !> @brief Advances the results data base to the next step.
  !>
  !> @param startTime Start time of the simulation
  !> @param currTime The time of current time step
  !> @param[in] stopTime Stop time of the simulation
  !> @param[in] timeInc The time amount to advance, negative means one step
  !> @param[out] iStep Time step index
  !> @return .true. if a new time styep was found, otherwise .false.
  !>
  !> @details This function advances the results data base,
  !> either by using a fixed time increment or just taking every time step
  !> stored between the specified start- and stop time.
  !>
  !> @author Knut Morten Okstad
  !> @date 6 June 2001

  function ffr_getNextStep (startTime,stopTime,timeInc,currTime,iStep)

    use PointerKindModule, only : dp, i8

    real(dp)   , intent(inout) :: startTime, currTime
    real(dp)   , intent(in)    :: stopTime, timeInc
    integer(i8), intent(out)   :: iStep
    logical                    :: ffr_getNextStep

    !! Local variables
    real(dp), parameter :: tolTime_p  = 1.0e-12_dp  ! The smallest step size
    real(dp), parameter :: hugeVal_p  = huge(1.0D0) ! The largest real number
    real(dp), save      :: lastTime   = -hugeVal_p  ! Time of the previous step

    !! --- Logic section ---

    if (currTime > stopTime-tolTime_p) then
       ffr_getNextStep = .false.
       return
    else if (currTime < startTime-tolTime_p) then
       !! Get the first time increment on file
       call ffr_setposition (startTime,currTime,iStep)
       startTime = currTime  ! Reset start time to the time actually found
       lastTime = -hugeVal_p ! Reset last time in case of several time loops
    else if (timeInc < tolTime_p) then
       !! Get the next time increment on file
       call ffr_increment (currTime,iStep)
    else
       !! We want constant time increments
       call ffr_setposition (currTime+timeInc,currTime,iStep)
    end if

    ffr_getNextStep = currTime < stopTime+tolTime_p .and. iStep >= 0_i8 .and. &
         &            currTime > lastTime+tolTime_p
    lastTime = currTime

  end function ffr_getNextStep

end module FFrExtractorInterface
