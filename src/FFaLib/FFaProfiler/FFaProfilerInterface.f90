!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

module FFaProfilerInterface

  implicit none

#ifndef FT_USE_PROFILER
  contains
#else
  interface
#endif

     subroutine ffa_newprofiler (name)
       character(len=*), intent(in) :: name
     end subroutine ffa_newprofiler

     subroutine ffa_starttimer (prog)
       character(len=*), intent(in) :: prog
     end subroutine ffa_starttimer

     subroutine ffa_stoptimer (prog)
       character(len=*), intent(in) :: prog
     end subroutine ffa_stoptimer

     subroutine ffa_reporttimer ()
     end subroutine ffa_reporttimer

     subroutine ffa_getmemusage (usage)
       integer , parameter   :: sp = kind(1.0)
       real(sp), intent(out) :: usage(4)
#ifndef FT_USE_PROFILER
       usage = 0.0_sp
#endif
     end subroutine ffa_getmemusage

     function ffa_getphysmem (wantTotal)
       logical, intent(in) :: wantTotal
       integer :: ffa_getphysmem
#ifndef FT_USE_PROFILER
       ffa_getphysmem = 0
#endif
     end function ffa_getphysmem

#ifdef FT_USE_PROFILER
  end interface

  private :: ffa_newprofiler


contains
#endif

  subroutine ffa_initprofiler (name)
    use FFaCmdLineArgInterface, only : ffa_cmdlinearg_isTrue
    character(len=*), intent(in) :: name
    if (ffa_cmdlinearg_isTrue('profile')) then
       call ffa_newprofiler (name)
    end if
  end subroutine ffa_initprofiler

  subroutine ffa_printMemStatus (lpu)
    integer, intent(in) :: lpu
    logical :: firstCall = .true.
    if (firstCall) then
       firstCall = .false.
       write(lpu,600) ffa_getphysmem(.false.), ffa_getphysmem(.true.)
    else
       write(lpu,601) ffa_getphysmem(.false.)
    end if
600 format(11X,'Current available memory:',I5,' MB      Total memory:',I5,' MB')
601 format(11X,'Current available memory:',I5,' MB')
  end subroutine ffa_printMemStatus

end module FFaProfilerInterface
