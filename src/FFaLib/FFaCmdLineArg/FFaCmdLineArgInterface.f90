!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaCmdLineArgInterface.f90
!> @brief Fortran interface for FFaCmdLineArg methods.
!> @details This file contains a module with interface definitions for some
!> methods of the FFaCmdLineArg class. See the file FFaCmdLineArg_F.C
!> for the implementation of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FFaCmdLineArg methods.

module FFaCmdLineArgInterface

  implicit none

  !> @cond NO_DOCUMENTATION
  interface

     subroutine ffa_cmdlinearg_init ()
     end subroutine ffa_cmdlinearg_init

     subroutine ffa_cmdlinearg_list (nodefault)
       logical, intent(in) :: nodefault
     end subroutine ffa_cmdlinearg_list

     pure subroutine ffa_cmdlinearg_getint (id,value)
       character*(*), intent(in)  :: id
       integer      , intent(out) :: value
     end subroutine ffa_cmdlinearg_getint

     pure subroutine ffa_cmdlinearg_getbool (id,value)
       character*(*), intent(in)  :: id
       logical      , intent(out) :: value
     end subroutine ffa_cmdlinearg_getbool

     pure subroutine ffa_cmdlinearg_getfloat (id,value)
       integer      , parameter   :: sp = kind(1.0)
       character*(*), intent(in)  :: id
       real(sp)     , intent(out) :: value
     end subroutine ffa_cmdlinearg_getfloat

     pure subroutine ffa_cmdlinearg_getdouble (id,value)
       integer      , parameter   :: dp = kind(1.0D0)
       character*(*), intent(in)  :: id
       real(dp)     , intent(out) :: value
     end subroutine ffa_cmdlinearg_getdouble

     subroutine ffa_cmdlinearg_getints (id,value,nval)
       character*(*), intent(in)  :: id
       integer      , intent(in)  :: nval
       integer      , intent(out) :: value(*)
     end subroutine ffa_cmdlinearg_getints

     subroutine ffa_cmdlinearg_getdoubles (id,value,nval)
       integer      , parameter   :: dp = kind(1.0D0)
       character*(*), intent(in)  :: id
       integer      , intent(in)  :: nval
       real(dp)     , intent(out) :: value(*)
     end subroutine ffa_cmdlinearg_getdoubles

     subroutine ffa_cmdlinearg_getstring (id,value)
       character*(*), intent(in)  :: id
       character*(*), intent(out) :: value
     end subroutine ffa_cmdlinearg_getstring

     pure function ffa_cmdlinearg_isSet (id)
       character*(*), intent(in)  :: id
       logical                    :: ffa_cmdlinearg_isSet
     end function ffa_cmdlinearg_isSet

  end interface
  !> @endcond

contains

  !> @brief Returns .true. if the command-line option @a id is specified.
  pure function FFa_CmdLineArg_isTrue (id) result(value)
    character*(*), intent(in) :: id
    logical                   :: value
    call ffa_cmdlinearg_getbool (id,value)
  end function FFa_CmdLineArg_isTrue

  !> @brief Returns the value of an integer command-line option @a id.
  pure function FFa_CmdLineArg_intValue (id) result(value)
    character*(*), intent(in) :: id
    integer                   :: value
    call ffa_cmdlinearg_getint (id,value)
  end function FFa_CmdLineArg_intValue

  !> @brief Returns the value of a single precision command-line option @a id.
  pure function FFa_CmdLineArg_floatValue (id) result(value)
    integer      , parameter  :: sp = kind(1.0)
    character*(*), intent(in) :: id
    real(sp)                  :: value
    call ffa_cmdlinearg_getfloat (id,value)
  end function FFa_CmdLineArg_floatValue

  !> @brief Returns the value of a double precision command-line option @a id.
  pure function FFa_CmdLineArg_doubleValue (id) result(value)
    integer      , parameter  :: dp = kind(1.0D0)
    character*(*), intent(in) :: id
    real(dp)                  :: value
    call ffa_cmdlinearg_getdouble (id,value)
  end function FFa_CmdLineArg_doubleValue

end module FFaCmdLineArgInterface
