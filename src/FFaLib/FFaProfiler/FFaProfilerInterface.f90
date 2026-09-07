!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaProfilerInterface.f90
!> @brief Fortran interface for FFaProfiler and FFaMemoryProfiler methods.
!> @details This file contains a module with Fortran interface definitions
!> for methods of the FFaProfiler and FFaMemoryProfiler classes.
!> See the files FFaProfiler_F.C and FFaMemoryProfiler_F.C
!> for the implementation of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FFaProfiler and FFaMemoryProfiler methods.

module FFaProfilerInterface

  implicit none

  interface

     !> @brief Returns the memory usage by current process.
     subroutine ffa_getmemusage (usage)
       integer , parameter   :: sp = kind(1.0)
       real(sp), intent(out) :: usage(4)
     end subroutine ffa_getmemusage

     !> @brief Returns the currently available or total physical memory.
     function ffa_getphysmem (wantTotal)
       logical, intent(in) :: wantTotal
       integer :: ffa_getphysmem
     end function ffa_getphysmem

#ifndef FT_USE_PROFILER
  end interface

  contains
#endif

     !> @brief Initializes the profiler object.
     subroutine ffa_newprofiler (name)
       character(len=*), intent(in) :: name
     end subroutine ffa_newprofiler

     !> @brief Starts profiling of a task named @a prog.
     subroutine ffa_starttimer (prog)
       character(len=*), intent(in) :: prog
     end subroutine ffa_starttimer

     !> @brief Stops profiling of the task named @a prog.
     subroutine ffa_stoptimer (prog)
       character(len=*), intent(in) :: prog
     end subroutine ffa_stoptimer

     !> @brief Prints out profiling summary and releases the profiler object.
     subroutine ffa_reporttimer ()
     end subroutine ffa_reporttimer

#ifdef FT_USE_PROFILER
  end interface

  private :: ffa_newprofiler


contains
#endif

  !> @brief Initializes the profiler object, if requested.
  subroutine ffa_initprofiler (name)
    use FFaCmdLineArgInterface, only : ffa_cmdlinearg_isTrue
    character(len=*), intent(in) :: name
    if (ffa_cmdlinearg_isTrue('profile')) then
       call ffa_newprofiler (name)
    end if
  end subroutine ffa_initprofiler

  !> @brief Prints the physical memory status to file unit @ lpu.
  subroutine ffa_printMemStatus (lpu)
    integer, intent(in) :: lpu
    logical :: firstCall = .true.
    if (firstCall) then
       firstCall = .false.
       write(lpu,600) ffa_getphysmem(.false.), ffa_getphysmem(.true.)
    else
       write(lpu,601) ffa_getphysmem(.false.)
    end if
600 format(11X,'Current available memory:',I6,' MB    Total memory:',I6,' MB')
601 format(11X,'Current available memory:',I6,' MB')
  end subroutine ffa_printMemStatus

end module FFaProfilerInterface
