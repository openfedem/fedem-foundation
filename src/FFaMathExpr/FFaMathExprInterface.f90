!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFaMathExprInterface.f90
!> @brief Fortran interface for FFaMathExprFactory methods.
!> @details This file contains a module with Fortran interface definitions
!> for the methods of the FFaMathExprFactory class.
!> See the file FFaMathExprFactory_F.C for the implementation
!> of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FFaMathExprFactory methods.

module FFaMathExprInterface

  implicit none

  interface

     !> @brief Creates a new expression function.
     !> @param[in] narg Number of function arguments
     !> @param[in] expression The math exression as a text string
     !> @param[in] exprId Identifier/key for the created expression
     !> @param[out] err Error flag
     subroutine ffame_create (narg,expression,exprId,err)
       integer     , intent(in)  :: narg
       character(*), intent(in)  :: expression
       integer     , intent(in)  :: exprId
       integer     , intent(out) :: err
     end subroutine ffame_create

     !> @brief Evaluates a single-argument expression function.
     !> @param[in] exprId Identifier/key for the expression to evaluate
     !> @param[in] arg Function argument value
     !> @param[out] err Error flag
     !> @return The function value
     function ffame_getvalue (exprId,arg,err)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp)              :: ffame_getvalue
       integer , intent(in)  :: exprId
       real(dp), intent(in)  :: arg
       integer , intent(out) :: err
     end function ffame_getvalue

     !> @brief Evaluates a multi-argument expression function.
     !> @param[in] exprId Identifier/key for the expression to evaluate
     !> @param[in] args Function argument values
     !> @param[out] err Error flag
     !> @return The function value
     function ffame_getvalue2 (exprId,args,err)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp)              :: ffame_getvalue2
       integer , intent(in)  :: exprId
       real(dp), intent(in)  :: args(*)
       integer , intent(out) :: err
     end function ffame_getvalue2

     !> @brief Evaluates derivative of a single-argument expression function.
     !> @param[in] exprId Identifier/key for the expression to evaluate
     !> @param[in] arg Function argument value
     !> @param[out] err Error flag
     !> @return The function derivative
     function ffame_getdiff (exprId,arg,err)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp)              :: ffame_getdiff
       integer , intent(in)  :: exprId
       real(dp), intent(in)  :: arg
       integer , intent(out) :: err
     end function ffame_getdiff

     !> @brief Evaluates a derivative of a multi-argument expression function.
     !> @param[in] exprId Identifier/key for the expression to evaluate
     !> @param[in] ia Index of the argument to return derivative with respect to
     !> @param[in] args Function argument values
     !> @param[out] err Error flag
     !> @return The function derivative
     function ffame_getdiff2 (exprId,ia,args,err)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp)              :: ffame_getdiff
       integer , intent(in)  :: exprId, ia
       real(dp), intent(in)  :: args(*)
       integer , intent(out) :: err
     end function ffame_getdiff2

  end interface

end module FFaMathExprInterface
