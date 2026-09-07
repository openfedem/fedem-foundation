!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaCmdLineArgInterface.f90
!> @brief Fortran interface for FFaCmdLineArg methods.
!> @details This file contains a module with Fortran interface definitions
!> for some methods of the FFaCmdLineArg class. See the file FFaCmdLineArg_F.C
!> for the implementation of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FFaCmdLineArg methods.

module FFaCmdLineArgInterface

  implicit none

  interface

     !> @brief Adds common command-line options used by all Fortran modules.
     subroutine ffa_cmdlinearg_init ()
     end subroutine ffa_cmdlinearg_init

     !> @brief Prints out all command-line options.
     !> @param[in] nodefault If .true., print only the sepcified options
     subroutine ffa_cmdlinearg_list (nodefault)
       logical, intent(in) :: nodefault
     end subroutine ffa_cmdlinearg_list

     !> @brief Returns actual value of specified integer command-line option.
     !> @param[in] id The command-line option to evaluate
     !> @param[out] value Specified (or default) value of the option @a id
     pure subroutine ffa_cmdlinearg_getint (id,value)
       character*(*), intent(in)  :: id
       integer      , intent(out) :: value
     end subroutine ffa_cmdlinearg_getint

     !> @brief Returns actual value of specified boolean command-line option.
     !> @param[in] id The command-line option to evaluate
     !> @param[out] value Specified (or default) value of the option @a id
     pure subroutine ffa_cmdlinearg_getbool (id,value)
       character*(*), intent(in)  :: id
       logical      , intent(out) :: value
     end subroutine ffa_cmdlinearg_getbool

     !> @brief Returns actual value of specified float command-line option.
     !> @param[in] id The command-line option to evaluate
     !> @param[out] value Specified (or default) value of the option @a id
     pure subroutine ffa_cmdlinearg_getfloat (id,value)
       integer      , parameter   :: sp = kind(1.0)
       character*(*), intent(in)  :: id
       real(sp)     , intent(out) :: value
     end subroutine ffa_cmdlinearg_getfloat

     !> @brief Returns actual value of specified double command-line option.
     !> @param[in] id The command-line option to evaluate
     !> @param[out] value Specified (or default) value of the option @a id
     pure subroutine ffa_cmdlinearg_getdouble (id,value)
       integer      , parameter   :: dp = kind(1.0D0)
       character*(*), intent(in)  :: id
       real(dp)     , intent(out) :: value
     end subroutine ffa_cmdlinearg_getdouble

     !> @brief Returns actual values of specified integer command-line option.
     !> @param[in] id The command-line option to evaluate
     !> @param[out] value List of specified values of the option @a id
     !> @param[in] nval Maximum number of values for the option @a id
     subroutine ffa_cmdlinearg_getints (id,value,nval)
       character*(*), intent(in)  :: id
       integer      , intent(in)  :: nval
       integer      , intent(out) :: value(*)
     end subroutine ffa_cmdlinearg_getints

     !> @brief Returns actual values of specified double command-line option.
     !> @param[in] id The command-line option to evaluate
     !> @param[out] value List of specified values of the option @a id
     !> @param[in] nval Maximum number of values for the option @a id
     subroutine ffa_cmdlinearg_getdoubles (id,value,nval)
       integer      , parameter   :: dp = kind(1.0D0)
       character*(*), intent(in)  :: id
       integer      , intent(in)  :: nval
       real(dp)     , intent(out) :: value(*)
     end subroutine ffa_cmdlinearg_getdoubles

     !> @brief Returns actual value of specified character command-line option.
     !> @param[in] id The command-line option to evaluate
     !> @param[out] value Specified (or default) value of the option @a id
     subroutine ffa_cmdlinearg_getstring (id,value)
       character*(*), intent(in)  :: id
       character*(*), intent(out) :: value
     end subroutine ffa_cmdlinearg_getstring

     !> @brief Returns .true. if the command-line option @a id is specified.
     pure function ffa_cmdlinearg_isSet (id)
       character*(*), intent(in)  :: id
       logical                    :: ffa_cmdlinearg_isSet
     end function ffa_cmdlinearg_isSet

  end interface

contains

  !> @brief Returns the value of an boolean command-line option @a id.
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
