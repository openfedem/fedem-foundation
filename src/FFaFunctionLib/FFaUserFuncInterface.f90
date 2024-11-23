!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaUserFuncInterface.f90
!> @brief Fortran interface for FFaUserFuncPlugin methods.
!> @details This file contains a module with Fortran interface definitions
!> for the methods of the FFaUserFuncPlugin class.
!> See the file FFaUserFuncPlugin_F.C for the implementation
!> of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FFaUserFuncPlugin methods.

module FFaUserFuncInterface

  implicit none

  interface

     !> @brief Loads the user-defined plugin(s) into core memory.
     !> @param[in] plugin List of user-defined plugin libraries
     !> @param[in] funcId Index of function to return number of arguments for
     !> @param[out] sign Signature string of the user-defined function plugin
     !> @return Number of arguments for specified user-defined function
     function ffauf_init (plugin,funcId,sign)
       integer                       :: ffauf_init
       character(len=*), intent(in)  :: plugin
       integer         , intent(in)  :: funcId
       character(len=*), intent(out) :: sign
     end function ffauf_init

     !> @brief Returns the number of parameters for the function @a funcId.
     function ffauf_getnopar (funcId)
       integer               :: ffauf_getnopar
       integer , intent(in)  :: funcId
     end function ffauf_getnopar

     !> @brief Returns the function flag for the function @a funcId.
     function ffauf_getflag (funcId)
       integer               :: ffauf_getflag
       integer , intent(in)  :: funcId
     end function ffauf_getflag

     !> @brief Evaluates the specified user-defined function.
     !> @param[in] baseId Base Id of the function to evaluate
     !> @param[in] funcId Index of function to evaluate
     !> @param[in] params Function parameters (constant)
     !> @param[in] args Function argument values
     !> @param[out] err Error flag
     !> @return The function value
     function ffauf_getvalue (baseId,funcId,params,args,err)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp)              :: ffauf_getvalue
       integer , intent(in)  :: baseId, funcId
       real(dp), intent(in)  :: params(*), args(*)
       integer , intent(out) :: err
     end function ffauf_getvalue

     !> @brief Evaluates the derivative of specified user-defined function.
     !> @param[in] baseId Base Id of the function to evaluate
     !> @param[in] funcId Index of function to evaluate
     !> @param[in] ia Index of the argument to return derivative with respect to
     !> @param[in] params Function parameters (constant)
     !> @param[in] args Function argument values
     !> @param[out] err Error flag
     !> @return The function derivative
     function ffauf_getdiff (baseId,funcId,ia,params,args,err)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp)              :: ffauf_getdiff
       integer , intent(in)  :: baseId, funcId, ia
       real(dp), intent(in)  :: params(*), args(*)
       integer , intent(out) :: err
     end function ffauf_getdiff

     !> @brief Evaluates the specified user-defined wave function.
     !> @param[in] baseId Base Id of the function to evaluate
     !> @param[in] funcId Index of function to evaluate
     !> @param[in] d Wather depth
     !> @param[in] g Gravity constant
     !> @param[in] params Function parameters (constant)
     !> @param[in] args Function argument values (position and time)
     !> @param[out] h Wave elevation
     !> @param[out] u Water particle velocity
     !> @param[out] du Water particle acceleration
     !> @return Error flag
     function ffauf_wave (baseId,funcId,d,g,params,args,h,u,du)
       integer , parameter   :: dp = kind(1.0D0)
       integer               :: ffauf_wave
       integer , intent(in)  :: baseId, funcId
       real(dp), intent(in)  :: d, g, params(*), args(*)
       real(dp), intent(out) :: h, u(3), du(3)
     end function ffauf_wave

  end interface

end module FFaUserFuncInterface
