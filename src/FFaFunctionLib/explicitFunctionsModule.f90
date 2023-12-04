!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

module ExplicitFunctionsModule

  !!============================================================================
  !    *    This module contains the routines                            *
  !    *                                                                 *
  !    *    funcValue : Administrator                                    *
  !    *    funcDerivate : Explicit differentiation                      *
  !    *    funcIntegral : Explicit integration                          *
  !    *    FUNC1  : SIN-function                                        *
  !    *    FUNC2  : Two SIN-functions summed, TMAX given                *
  !    *    FUNC3  : Two SIN-functions summed, TMIN given                *
  !    *    FUNC4  : Wave function (SIN-functions with random phase)     *
  !    *    FUNC5  : Servo simulator                                     *
  !    *    FUNC6  : Spline function                                     *
  !    *    FUNC8  : Streamline wave function                            *
  !    *    FUNC9  : Wave function with embedded streamline functions    *
  !    *    FUNC10 : Piece-wise linear function                          *
  !    *    FUNC11 : Ramp function                                       *
  !    *    FUNC12 : Step function                                       *
  !    *    FUNC13 : Square pulse function                               *
  !    *    FUNC14 : Dirac pulse function                                *
  !    *    FUNC15 : Constant function                                   *
  !    *    FUNC16 : Extended ramp function                              *
  !    *    FUNC17 : Scaling function                                    *
  !    *    FUNC18 : External device function                            *
  !    *    FUNC19 : General math expression function                    *
  !    *    FUNC20 : Multi-variable math expression function             *
  !    *    FUNC21 : Multi-variable user-defined function (plug-in)      *
  !    *    FUNC22 : Smooth trajectory based on sin2                     *
  !    *                                                                 *
  !    *    Trondhjem 84-05-10 / OIS                                     *
  !    *    Trondheim 99-09-05 / Modified for f90 by BH                  *
  !!============================================================================

  implicit none

  integer, private, parameter :: dp = kind(1.0D0) ! 8-byte real (double)

  real(dp),private, parameter :: PI = 3.141592653589793238_dp
  real(dp),private, parameter :: twoPI = PI+PI
  real(dp),private, parameter :: epsArg_p = 1.0e-15_dp ! Argument tolerance

  integer, parameter :: maxFunc_p = 22
  integer, parameter :: nSpln_p = 6

  integer, parameter :: SINUSOIDAL_p          =  1, &
       &                COMPL_SINUS_p         =  2, &
       &                DELAYED_COMPL_SINUS_p =  3, &
       &                WAVE_SINUS_p          =  4, &
       &                LIN_VEL_VAR_p         =  5, &
       &                SPLINE_p              =  6, &
       &                WAVE_STOKES5_p        =  7, &
       &                WAVE_STREAMLINE_p     =  8, &
       &                WAVE_EMBEDDED_p       =  9, &
       &                LIN_VAR_p             = 10, &
       &                RAMP_p                = 11, &
       &                STEP_p                = 12, &
       &                SQUARE_PULS_p         = 13, &
       &                DIRAC_PULS_p          = 14, &
       &                CONSTANT_p            = 15, &
       &                LIM_RAMP_p            = 16, &
       &                SCALE_p               = 17, &
       &                DEVICE_FUNCTION_p     = 18, &
       &                MATH_EXPRESSION_p     = 19, &
       &                USER_DEFINED_p        = 21, &
       &                SMOOTH_TRAJ_p         = 22

  character(len=19), parameter:: funcType_p(maxFunc_p) = (/  &
       &               'SINUSOIDAL         ',  & !  1
       &               'COMPL_SINUS        ',  & !  2
       &               'DELAYED_COMPL_SINUS',  & !  3
       &               'WAVE_SINUS         ',  & !  4
       &               'LIN_VEL_VAR        ',  & !  5
       &               'SPLINE             ',  & !  6
       &               'WAVE_STOKES5       ',  & !  7
       &               'WAVE_STREAMLINE    ',  & !  8
       &               'WAVE_EMBEDDED      ',  & !  9
       &               'LIN_VAR            ',  & ! 10
       &               'RAMP               ',  & ! 11
       &               'STEP               ',  & ! 12
       &               'SQUARE_PULS        ',  & ! 13
       &               'DIRAC_PULS         ',  & ! 14
       &               'CONSTANT           ',  & ! 15
       &               'LIM_RAMP           ',  & ! 16
       &               'SCALE              ',  & ! 17
       &               'DEVICE_FUNCTION    ',  & ! 18
       &               'MATH_EXPRESSION    ',  & ! 19
       &               '-------------------',  & ! 20
       &               'USER_DEFINED       ',  & ! 21
       &               'SMOOTH_TRAJ        ' /)  ! 22

  integer, save :: dbgFunc = 0

  type SplineData
     integer           :: N
     real(dp), pointer :: X(:), Y(:), DERIV(:,:)
  end type SplineData

  type(SplineData), save :: spl(nSpln_p)
  integer         , save :: last(nSpln_p) = 0


  interface funcValue
     module procedure funcValue1
     module procedure funcValue2
  end interface

  interface funcDerivative
     module procedure funcDerivative1
     module procedure funcDerivative2
  end interface

  private :: SplineData, spl, SPLIN3, outOfRangeError
  private :: RampIntegral, func10diff, func10integral, func18integral
  private :: funcValue1, funcValue2, funcDerivative1, funcDerivative2


contains

  subroutine funcValue1 (ikf,ifunc,rfunc,x,fn,ierr)

    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran subroutine
    !  *
    !  * PURPOSE          : Call appropriate explicit function.
    !  *
    !  * ENTRY ARGUMENTS  : Argument...................................X
    !  *                    Function identification number...........IKF
    !  *                    Integer function input parameters......IFUNC
    !  *                    Real function input parameters.........RFUNC
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time X...............FN
    !  *                    Error indicator.........................IERR
    !  *
    !  * CALLING SEQUENCE : CALL funcValue(IKF,IFUNC,RFUNC,X,FN,IERR)
    !  *
    !  * ERROR INDICATION : IERR < 0  -->  Error has occurred
    !  *
    !  * AUTHOR           : Ole Ivar Sivertsen   1989
    !  *                    SINTEF avd. for maskinkonstruksjon
    !***********************************************************************

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: ikf, ifunc(:)
    real(dp), intent(in)    :: rfunc(:), x
    real(dp), intent(out)   :: fn
    integer , intent(inout) :: ierr

    !! Local variables
    integer  :: lerr, np
    real(dp) :: args(1)

    !! --- Logic section ---

    lerr = ierr

    select case (ifunc(1))

    case(SINUSOIDAL_p)
       FN = FUNC1(ifunc,rfunc,X,0.0_dp)

    case(COMPL_SINUS_p)
       FN = FUNC2(rfunc,X,0.0_dp)

    case(DELAYED_COMPL_SINUS_p)
       FN = FUNC3(rfunc,X,0.0_dp)

    case(WAVE_SINUS_p)
       FN = FUNC4(ifunc,rfunc,X,0.0_dp,IERR)

    case(LIN_VEL_VAR_p)
       FN = FUNC5(IKF,ifunc,rfunc,X,IERR)

    case(SPLINE_p)
       np = size(rfunc)/2
       FN = FUNC6(IKF,ifunc,reshape(rfunc,(/2,np/)),X,IERR)

    case(WAVE_STOKES5_p,WAVE_STREAMLINE_p)
       FN = FUNC8(rfunc,X,0.0_dp,IERR)

    case(WAVE_EMBEDDED_p)
       FN = FUNC9(ifunc,rfunc,X,0.0_dp,IERR)

    case(LIN_VAR_p)
       np = size(rfunc)/2
       FN = FUNC10(IKF,ifunc,reshape(rfunc,(/2,np/)),X,IERR)

    case(RAMP_p)
       FN = FUNC11(rfunc,X)

    case(STEP_p)
       FN = FUNC12(rfunc,X)

    case(SQUARE_PULS_p)
       FN = FUNC13(rfunc,X)

    case(DIRAC_PULS_p)
       FN = FUNC14(rfunc,X)

    case(CONSTANT_p)
       FN = FUNC15(rfunc)

    case(LIM_RAMP_p)
       FN = FUNC16(rfunc,X)

    case(SCALE_p)
       FN = FUNC17(rfunc,X)

    case(DEVICE_FUNCTION_p)
       FN = FUNC18(IKF,ifunc,rfunc,X,IERR)

    case(MATH_EXPRESSION_p)
       FN = FUNC19(IKF,ifunc,X,IERR)

    case(USER_DEFINED_p)
       args(1) = X
       FN = FUNC21(IKF,ifunc,rfunc,args,IERR)

    case(SMOOTH_TRAJ_p)
       FN = FUNC22(IKF,rfunc,X,IERR)

    case default
       fn = 0.0_dp
       ierr = ierr - 1
       call ffamsg_list ('Error :   Function type not implemented:',ifunc(1))
    end select

    if (ierr < lerr) then
       call ffamsg_list ('Debug :   funcValue')
    else if (dbgFunc > 0) then
       if (X > -huge(1.0_dp)) then
          write(dbgFunc,600) funcType_p(ifunc(1)),IKF,X,FN
       else ! Function with no argument (constant value)
          write(dbgFunc,601) funcType_p(ifunc(1)),IKF,FN
       end if
600    format('funcValue: ',A15,', IKF =',I6,' X, F =',1P2E12.5)
601    format('funcValue: ',A15,', IKF =',I6,'    F =',1P,E12.5)
    end if

  end subroutine funcValue1


  subroutine funcValue2 (ikf,ifunc,rfunc,x,fn,ierr)

    !!==========================================================================
    !! Evaluation of function with (possibly) multi-dimensional argument.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : October 2009 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: ikf, ifunc(:)
    real(dp), intent(in)    :: rfunc(:), x(:)
    real(dp), intent(out)   :: fn
    integer , intent(inout) :: ierr

    !! Local variables
    integer :: lerr

    !! --- Logic section ---

    lerr = ierr

    if (ifunc(1) == USER_DEFINED_p) then
       fn = func21(ikf,ifunc,rfunc,x,ierr)
    else if (size(x) > 1) then
       select case (ifunc(1))
       case (SINUSOIDAL_p)
          FN = FUNC1(ifunc,rfunc,X(1),X(2))
       case (COMPL_SINUS_p)
          FN = FUNC2(rfunc,X(1),X(2))
       case (DELAYED_COMPL_SINUS_p)
          FN = FUNC3(rfunc,X(1),X(2))
       case (WAVE_SINUS_p)
          FN = FUNC4(ifunc,rfunc,X(1),X(2),IERR)
       case (WAVE_STOKES5_p,WAVE_STREAMLINE_p)
          FN = FUNC8(rfunc,X(1),X(2),IERR)
       case (WAVE_EMBEDDED_p)
          FN = FUNC9(ifunc,rfunc,X(1),X(2),IERR)
       case (MATH_EXPRESSION_p)
          fn = func20(ikf,ifunc,X,IERR)
       case default
          call funcValue1 (ikf,ifunc,rfunc,x(1),fn,ierr)
       end select
    else
       call funcValue1 (ikf,ifunc,rfunc,x(1),fn,ierr)
       return
    end if

    if (ierr < lerr) then
       call ffamsg_list ('Debug :   funcValue')
    else if (dbgFunc > 0) then
       write(dbgFunc,600) funcType_p(ifunc(1)),IKF,X,FN
600    format('funcValue: ',A15,', IKF =',I6,' X, F =',1P6E12.5)
    end if

  end subroutine funcValue2


  subroutine funcDerivative1 (ikf,iorder,ifunc,rfunc,x,fn,ierr)

    !!==========================================================================
    !! Explicit differentiation of the given function at point x.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : December 2005 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)  :: ikf, iorder, ifunc(:)
    real(dp), intent(in)  :: rfunc(:), x
    real(dp), intent(out) :: fn
    integer , intent(out) :: ierr

    !! Local variables
    integer  :: np
    real(dp) :: args(1)

    !! --- Logic section ---

    ierr = 0
    fn = 0.0_dp

    select case (ifunc(1))

    case (CONSTANT_p)
       if (iorder == 0) then
          fn = rfunc(1)
       end if

    case (SCALE_p)
       if (iorder == 1) then
          fn = rfunc(1)
       else if (iorder == 0) then
          fn = rfunc(1) * X
       end if

    case (LIN_VAR_p)

       np = size(rfunc)/2
       if (iorder == 1) then
          FN = FUNC10diff(ikf,ifunc,reshape(rfunc,(/2,np/)),x,ierr)
       else if (iorder == 0) then
          FN = FUNC10(ikf,ifunc,reshape(rfunc,(/2,np/)),x,ierr)
       end if

    case (RAMP_p)
       if (iorder == 1) then
          if (X < rfunc(3)) then
             fn = 0.0_dp
          else if (X > rfunc(3)) then
             fn = rfunc(2)
          else
             ierr = 1
          end if
       else if (iorder == 0) then
          fn = FUNC11(rfunc,x)
       end if

    case (LIM_RAMP_p)
       if (iorder == 1) then
          if (X < rfunc(3) .or. X > rfunc(4)) then
             fn = 0.0_dp
          else if (X > rfunc(3) .and. X < rfunc(4)) then
             fn = rfunc(2)
          else
             ierr = 1
          end if
       else if (iorder == 0) then
          fn = FUNC16(rfunc,x)
       end if

    case (MATH_EXPRESSION_p)
       if (iorder == 1) then
          fn = func19diff(ikf,ifunc,x,ierr)
       else if (iorder == 0) then
          fn = func19(ikf,ifunc,x,ierr)
       end if

    case (USER_DEFINED_p)
       args(1) = x
       if (iorder == 1) then
          fn = func21diff(ikf,ifunc,rfunc,args,ierr)
       else if (iorder == 0) then
          fn = func21(ikf,ifunc,rfunc,args,ierr)
       end if

    case default
       ierr = 1 ! Explicit differentiation not available
    end select

    if (ierr < 0) then
       call ffamsg_list ('Debug :   funcDerivative')
    else if (dbgFunc > 0 .and. ierr == 0 .and. iorder == 1) then
       write(dbgFunc,600) trim(funcType_p(ifunc(1))),IKF,X,FN
600    format('funcDerivative1: ',A,', IKF =',I6,' X, dF/dX =',1P2E12.5)
    end if

  end subroutine funcDerivative1


  subroutine funcDerivative2 (ikf,iorder,ifunc,rfunc,x,fn,ierr)

    !!==========================================================================
    !! Explicit differentiation of function with multi-dimensional argument.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : October 2009 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)  :: ikf, iorder, ifunc(:)
    real(dp), intent(in)  :: rfunc(:), x(:)
    real(dp), intent(out) :: fn
    integer , intent(out) :: ierr

    !! --- Logic section ---

    if (ifunc(1) == MATH_EXPRESSION_p .and. size(x) > 1) then
       if (iorder == 1) then
          fn = func20diff(ikf,ifunc,x,ierr)
       else if (iorder == 0) then
          fn = func20(ikf,ifunc,x,ierr)
       end if
    else if (ifunc(1) == USER_DEFINED_p) then
       if (iorder == 1) then
          fn = func21diff(ikf,ifunc,rfunc,x,ierr)
       else if (iorder == 0) then
          fn = func21(ikf,ifunc,rfunc,x,ierr)
       end if
    else
       call funcDerivative1 (ikf,iorder,ifunc,rfunc,x(1),fn,ierr)
    end if

    if (ierr < 0) call ffamsg_list ('Debug :   funcDerivative2')

  end subroutine funcDerivative2


  subroutine funcIntegral (ikf,iorder,ifunc,rfunc,x,fn,ierr)

    !!==========================================================================
    !! Explicit integration of the given function from 0.0 to x.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : December 2004 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: ikf, iorder, ifunc(:)
    real(dp), intent(in)    :: rfunc(:), x
    real(dp), intent(out)   :: fn
    integer , intent(inout) :: ierr

    !! Local variables
    integer :: lerr, np

    !! --- Logic section ---

    lerr = ierr

    select case (ifunc(1))

    case (CONSTANT_p)
       if (iorder == 1) then
          FN = rfunc(1) * X
       else if (iorder == 2) then
          FN = rfunc(1) * X*X / 2.0_dp
       else
          FN = rfunc(1)
       end if

    case (SCALE_p)
       if (iorder == 1) then
          FN = rfunc(1) * X*X / 2.0_dp
       else if (iorder == 2) then
          FN = rfunc(1) * X*X*X / 6.0_dp
       else
          FN = rfunc(1) * X
       end if

    case (RAMP_p)
       FN = RampIntegral(iorder,X,rfunc(1),rfunc(2),rfunc(3))

    case (LIM_RAMP_p)
       FN = RampIntegral(iorder,X,rfunc(1),rfunc(2),rfunc(3),rfunc(4))

    case (LIN_VAR_p)
       np = size(rfunc)/2
       FN = FUNC10integral(ikf,ifunc,reshape(rfunc,(/2,np/)),X,iorder,ierr)

    case (DEVICE_FUNCTION_p)
       FN = FUNC18integral(ikf,ifunc,rfunc,X,iorder,ierr)

    case default
       fn = 0.0_dp
       ierr = ierr - 1
       call ffamsg_list ('Error :   Function type not implemented:',ifunc(1))
    end select

    if (ierr < lerr) then
       call ffamsg_list ('Debug :   funcIntegral')
    else if (dbgFunc > 0 .and. iorder >= 0 .and. iorder <= 2) then
       write(dbgFunc,600) trim(funcType_p(ifunc(1))),IKF,iorder,X,FN
600    format('funcIntegral: ',A,', IKF, IORDER =',I6,I2, &
            & ' X, Int(F)dX =',1P2E12.5)
    end if

  end subroutine funcIntegral


  function CanIntegrateExplicitly (funcType,intOrder) result(CanInt)

    !!==========================================================================
    !! Query function telling if a function can be integrated explicitly.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : December 2004 / 2.0
    !!==========================================================================

    integer, intent(in) :: funcType, intOrder
    logical             :: CanInt

    !! --- Logic section ---

    select case (funcType)
    case (CONSTANT_p, SCALE_p, LIN_VAR_p, DEVICE_FUNCTION_p)
       CanInt = intOrder >= 0 .and. intOrder <= 2
    case (RAMP_p, LIM_RAMP_p)
       CanInt = intOrder >= 1 .and. intOrder <= 2
    case default
       CanInt = .false.
    end select

  end function CanIntegrateExplicitly


  subroutine outOfRangeError (X,X0,X1)
    use FFaMsgInterface, only : ffamsg_list
    real(dp), intent(in) :: X, X0, X1
    character(len=64)    :: errMsg
    write(errMsg,"('x =',1PE12.5,', range = [ ',E12.5,',',E12.5,' ]')") X,X0,X1
    call ffamsg_list ('Error :   Function argument is out of range')
    call ffamsg_list ('          '//trim(errMsg))
  end subroutine outOfRangeError


  !***********************************************************************
  !************************************************************  FUNC1   *
  !***********************************************************************

  function FUNC1 (IFUNC,RFUNC,T,X)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 1
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a simple sinusoidal function.
    !  *                    A sine wave can be delayed and displaced.
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Position...................................X
    !  *                    Period velocity.......................REVSEC
    !  *                    Period delay..........................REVDEL
    !  *                    Amplitude................................AMP
    !  *                    Amplitude displacement.................DELTA
    !  *                    Maximum time for the function to apply..TMAX
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC1(IFUNC,RFUNC,T,X)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Ole Ivar Sivertsen   1984
    !  *                    SINTEF avd. for maskinkonstruksjon
    !***********************************************************************

    integer , intent(in)    :: IFUNC(3)
    real(dp), intent(in)    :: RFUNC(:), T, X

    !! Local variables
    real(dp) :: OMEGA, EPS, AMP, DELTA, TMAX, TIME, XoG, FUNC1

    !! --- Logic section ---

    if (IFUNC(3) == 2) then
       AMP   = RFUNC(1)
       OMEGA = RFUNC(2)
       EPS   =-RFUNC(3)
    else
       OMEGA = RFUNC(1)*twoPI
       EPS   = RFUNC(2)*twoPI
       AMP   = RFUNC(3)
    end if
    DELTA  = RFUNC(4)
    TMAX   = RFUNC(5)
    if (TMAX > 0.0_dp .and. T > TMAX) then
       TIME = TMAX
    else
       TIME = T
    end if
    if (size(RFUNC) < 6) then
       XoG = 0.0_dp
    else if (IFUNC(3) == 2) then
       XoG = -X/RFUNC(6)
    else
       XoG = X/RFUNC(6)
    end if

    FUNC1  = AMP*sin(OMEGA*(TIME+OMEGA*XoG)-EPS) + DELTA

  end function FUNC1


  !***********************************************************************
  !************************************************************  FUNC2   *
  !***********************************************************************

  function FUNC2 (RFUNC,T,X)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 2
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a complex sinusoidal made up of
    !  *                    2 sine waves which separately can be delayed.
    !  *                    The sine waves are added together and a total
    !  *                    dispacement can be specified.
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Position...................................X
    !  *                    Period velocity  wave1...............REVSEC1
    !  *                    Period velocity  wave2...............REVSEC2
    !  *                    Period delay  wave1..................REVDEL1
    !  *                    Period delay  wave2..................REVDEL2
    !  *                    Amplitude  wave1........................AMP1
    !  *                    Amplitude  wave2........................AMP2
    !  *                    Amplitude displacement.................DELTA
    !  *                    Maximum time for the function to apply..TMAX
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC2(RFUNC,T,X)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Sjur Lassesen        1986
    !  *                    SINTEF avd. for maskinkonstruksjon
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(:), T, X

    !! Local variables
    real(dp) :: OMEGA(2), EPS(2), AMP(2), DELTA, TMAX, TIME, F1, F2, FUNC2
    real(dp) :: XoG

    !! --- Logic section ---

    OMEGA  = RFUNC(1:2)*twoPI
    EPS    = RFUNC(3:4)*twoPI
    AMP    = RFUNC(5:6)
    DELTA  = RFUNC(7)
    TMAX   = RFUNC(8)
    if (TMAX > 0.0_dp .and. T > TMAX) then
       TIME = TMAX
    else
       TIME = T
    end if
    if (size(RFUNC) > 8) then
       XoG = X/RFUNC(9)
    else
       XoG = 0.0_dp
    end if

    F1 = AMP(1)*sin(OMEGA(1)*(TIME+OMEGA(1)*XoG)-EPS(1)) + DELTA
    F2 = AMP(2)*sin(OMEGA(2)*(TIME+OMEGA(2)*XoG)-EPS(2))

    FUNC2 = F1 + F2

  end function FUNC2


  !***********************************************************************
  !************************************************************  FUNC3   *
  !***********************************************************************

  function FUNC3 (RFUNC,T,X)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 3
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a complex sinusoidal made up of
    !  *                    2 sine waves which separately can be delayed.
    !  *                    The sine waves are added together and a total
    !  *                    dispacement can be specified.
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Position...................................X
    !  *                    Period velocity  wave1...............REVSEC1
    !  *                    Period velocity  wave2...............REVSEC2
    !  *                    Period delay  wave1..................REVDEL1
    !  *                    Period delay  wave2..................REVDEL2
    !  *                    Amplitude  wave1........................AMP1
    !  *                    Amplitude  wave2........................AMP2
    !  *                    Amplitude displacement.................DELTA
    !  *                    Starting point for function to apply....TMIN
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC3(RFUNC,T,X)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Sjur Lassesen        1987
    !  *                    SINTEF avd. for maskinkonstruksjon
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(:), T, X

    !! Local variables
    real(dp) :: OMEGA(2), EPS(2), AMP(2), DELTA, TMIN, TIME, F1, F2, FUNC3
    real(dp) :: XoG

    !! --- Logic section ---

    OMEGA  = RFUNC(1:2)*twoPI
    EPS    = RFUNC(3:4)*twoPI
    AMP    = RFUNC(5:6)
    DELTA  = RFUNC(7)
    TMIN   = RFUNC(8)
    TIME   = max(T,TMIN)
    if (size(RFUNC) > 8) then
       XoG = X/RFUNC(9)
    else
       XoG = 0.0_dp
    end if

    F1 = AMP(1)*sin(OMEGA(1)*(TIME+OMEGA(1)*XoG)-EPS(1)) + DELTA
    F2 = AMP(2)*sin(OMEGA(2)*(TIME+OMEGA(2)*XoG)-EPS(2))

    FUNC3 = F1 + F2

  end function FUNC3


  !***********************************************************************
  !************************************************************  FUNC4   *
  !***********************************************************************

  function FUNC4 (IFUNC,RFUNC,T,X,IERR) result(F)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 4
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a complex sinusoidal made up of
    !  *                    an arbitrary number of sine waves with separate
    !  *                    amplitudes, frequencies and phase shifts.
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Position...................................X
    !  *                    Number of sine waves, N .............IFUNC(3)
    !  *                    Amplitude, A(i) ...................RFUNC(1,i)
    !  *                    Angular frequency, omega(i) .......RFUNC(2,i)
    !  *                    Phase shift, epsilon(i) ...........RFUNC(3,i)
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *                    Error indicator.........................IERR
    !  *
    !  * CALLING SEQUENCE : FN=FUNC4(IFUNC,RFUNC,T,X,IERR)
    !  *
    !  * ERROR INDICATION : IERR
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Knut Morten Okstad               16 January 2007
    !***********************************************************************

    integer , intent(in)    :: IFUNC(:)
    real(dp), intent(in)    :: RFUNC(:), T, X
    integer , intent(inout) :: IERR

    !! Local variables
    integer  :: j, M, N, MN1
    real(dp) :: F, XoG

    !! --- Logic section ---

    F = 0.0_dp
    N = IFUNC(3)
    M = size(RFUNC)/N
    if (M < 3) then
       IERR = IERR - 1
       return
    else if (size(RFUNC) > M*N .and. M == 3) then
       XoG = X/RFUNC(M*N+1)
    else
       XoG = 0.0_dp
    end if

    MN1 = M*N - M
    if (M > 3) then
       !$OMP PARALLEL DO PRIVATE(j),REDUCTION(+:F)
       do j = 0, MN1, M
          F = F + RFUNC(j+1)*SIN(RFUNC(j+2)*T-RFUNC(j+4)*X+RFUNC(j+3))
       end do
       !$OMP END PARALLEL DO
    else if (abs(XoG) <= epsArg_p) then
       !$OMP PARALLEL DO PRIVATE(j),REDUCTION(+:F)
       do j = 0, MN1, M
          F = F + RFUNC(j+1)*SIN(RFUNC(j+2)*T+RFUNC(j+3))
       end do
       !$OMP END PARALLEL DO
    else
       !$OMP PARALLEL DO PRIVATE(j),REDUCTION(+:F)
       do j = 0, MN1, M
          F = F + RFUNC(j+1)*SIN(RFUNC(j+2)*(T-RFUNC(j+2)*XoG)+RFUNC(j+3))
       end do
       !$OMP END PARALLEL DO
    end if

  end function FUNC4


  !***********************************************************************
  !************************************************************  FUNC5   *
  !***********************************************************************

  function FUNC5 (IKF,IFUNC,RFUNC,constT,IERR) result(FN)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 5
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a curve starting in 0.0 and varying
    !  *                    linearly between -1.0 and 1.0.
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Time incident for change in gradient.....RF1
    !  *                     .       .     .    .    .    .           .
    !  *                    Time incident for change in gradient.....RF8
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T...............FN
    !  *                    Error indicator.........................IERR
    !  *
    !  * CALLING SEQUENCE : FN=FUNC5(IKF,IFUNC,RFUNC,T,IERR)
    !  *
    !  * ERROR INDICATION : IERR < 0  -->  Error has occurred
    !  *
    !  *           NNN =  5        Negative starting time
    !  *           NNN = 10        2. change in grad. earlier than 1.
    !  *           NNN = 15        3. change in grad. earlier than 2.
    !  *           NNN = 20        4. change in grad. earlier than 3.
    !  *           NNN = 25        5. change in grad. earlier than 4.
    !  *           NNN = 30        6. change in grad. earlier than 5.
    !  *           NNN = 35        7. change in grad. earlier than 6.
    !  *           NNN = 40        8. change in grad. earlier than 7.
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Ole Ivar Sivertsen   1984
    !  *                    SINTEF avd. for maskinkonstruksjon
    !***********************************************************************

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: IKF, IFUNC(:)
    real(dp), intent(in)    :: RFUNC(:), constT
    integer , intent(inout) :: IERR

    !! Local variables
    integer  :: I, J, N, NNN, extrapolate
    logical  :: calcPastEnd
    real(dp) :: RF(8), RF1, RF2, RF3, RF4, RF5, RF6, RF7, RF8, RL, T, FN

    !! --- Logic section ---

    T  = constT
    FN = 0.0_dp
    N  = size(rfunc)

    calcPastEnd = .false.
    extrapolate = IFUNC(2)
    if (extrapolate > 0) then
       if (T < 0.0_dp .or. N < 1) then
          return
       else if (constT > RFUNC(N)) then
          calcPastEnd = .true.
          T = RFUNC(N)
       end if
    end if

    RL = 0.0_dp
    do I = 1, N, 8
       J = MIN(I+7,N)
       RF = -1.0_dp
       RF(1:1+J-I) = RFUNC(I:J)
       RF1 = RFUNC(1)
       RF2 = RFUNC(2)
       RF3 = RFUNC(3)
       RF4 = RFUNC(4)
       RF5 = RFUNC(5)
       RF6 = RFUNC(6)
       RF7 = RFUNC(7)
       RF8 = RFUNC(8)

       NNN = 5
       if (RF1 > RL)                               GOTO 915
       if (T <= RF1)                               exit

       NNN = 10
       if (RF1 > RF2)                              GOTO 915
       if (T <= RF2)                               then
          FN = FN + 0.5_dp*(T-RF1)*(T-RF1)/(RF2-RF1)
          exit
       end if

       NNN = 15
       if (RF2 > RF3)                              GOTO 915
       if (T <= RF3)                               then
          FN = FN + T - 0.5_dp*(RF2+RF1)
          exit
       end if

       NNN = 20
       if (RF3 > RF4)                              GOTO 915
       if (T <= RF4)                               then
          FN = FN + T - 0.5_dp*((T-RF3)*(T-RF3)/(RF4-RF3) + (RF2+RF1))
          exit
       end if

       NNN = 25
       if (RF4 > RF5)                              GOTO 915
       if (T <= RF5)                               then
          FN = FN + RF4 - 0.5_dp*((RF4-RF3) + (RF2+RF1))
          exit
       end if

       NNN = 30
       if (RF5 > RF6)                              GOTO 915
       if (T <= RF6)                               then
          FN = FN + RF4 - 0.5_dp*((RF4-RF3) + (RF2+RF1) + &
               &                  (T-RF5)*(T-RF5)/(RF6-RF5))
          exit
       end if

       NNN = 35
       if (RF6 > RF7)                              GOTO 915
       if (T <= RF7)                               then
          FN = FN + RF4 - 0.5_dp*((RF4-RF3) + (RF2+RF1) - (RF6+RF5)) - T
          exit
       end if

       NNN = 40
       if (RF7 > RF8)                              GOTO 915
       if (T <= RF8)                               then
          FN = FN + RF4 - 0.5_dp*((RF4-RF3) + (RF2+RF1) - (RF6+RF5) - &
               &                  (T-RF7)*(T-RF7)/(RF8-RF7)) - T
          exit
       end if

       FN = FN + RF4 - 0.5_dp*((RF4-RF3) + (RF2+RF1) - (RF6+RF5) - &
            &                  (RF8-RF7)) - RF8
       RL = RF8

    end do

    if (calcPastEnd .and. extrapolate == 2) then
       select case (NNN)
       case (10,40)
          FN = FN + constT - T
       case (20,30)
          FN = FN - constT + T
       end select
    end if

    return

915 IERR = IERR - NNN
    call ffamsg_list ('Error :   Could not evaluate servo simulator, NNN =',NNN)
    call ffamsg_list ('          FUNC5, baseId =',ikf)

  end function FUNC5


  !***********************************************************************
  !************************************************************  FUNC 6  *
  !***********************************************************************

  function FUNC6 (IKF,IFUNC,RFUNC,T,IERR)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 6
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : Computes a spline approximation of third order.
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    X(i) .........................RFUNC(2*i-1,j)
    !  *                    Y(X) .........................RFUNC(2*i  ,j)
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *                    Error indicator.........................IERR
    !  *
    !  * CALLING SEQUENCE : FN=FUNC6(IKF,IFUNC,RFUNC,T,IERR)
    !  *
    !  * ERROR INDICATION : IERR
    !  *
    !  * EXTERNAL CALLS   : SPLIN3
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Ole Ivar Sivertsen   1984
    !  *                    SINTEF avd. for maskinkonstruksjon
    !***********************************************************************

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: IKF, IFUNC(:)
    real(dp), intent(in)    :: RFUNC(:,:), T
    integer , intent(inout) :: IERR
    real(dp)                :: FUNC6

    !! Local variables
    integer :: iSpln, extrapolate, N, IERR2, IOP
    logical :: calcPastEnd, reAllocate

    integer, parameter :: M_p = 1
    real(dp) :: Z(M_p), FVALUE(M_p), FDERIV(M_p,2)

    !! --- Logic section ---

    FUNC6 = 0.0_dp
    N     = size(RFUNC,2)
    if (N < 1) return

    extrapolate = IFUNC(2)
    iSpln       = IFUNC(3)

    Z(1) = T
    calcPastEnd = .false.
    if (T < RFUNC(1,1) .or. T > RFUNC(1,N)) then
       if (extrapolate > 0) then
          calcPastEnd = .true.
          if (T < RFUNC(1,1)) Z(1) = RFUNC(1,1)
          if (T > RFUNC(1,N)) Z(1) = RFUNC(1,N)
       else if (T < RFUNC(1,1) .and. RFUNC(1,1)-T < epsArg_p) then
          !! TT #2729: Avoid out-of-range error due to round-off errors, etc.
          Z(1) = RFUNC(1,1)
       else if (T > RFUNC(1,N) .and. T-RFUNC(1,N) < epsArg_p) then
          !! TT #2729: Avoid out-of-range error due to round-off errors, etc.
          Z(1) = RFUNC(1,N)
       else
          call outOfRangeError (T,RFUNC(1,1),RFUNC(1,N))
          GOTO 915
       end if
    end if

    !! If iSpln is -1 we force a new initialization (in iSpln = 1)
    if (iSpln == -1) then
       if (last(1) > 0) last(1) = 1+IKF
       iSpln = 1
    end if

    if (last(iSpln) == IKF) then
       IOP = 1
    else
       reAllocate = .true.
       if (last(iSpln) > 0) then
          if (size(spl(iSpln)%X) /= N) then
             deallocate(spl(iSpln)%X,spl(iSpln)%Y,spl(iSpln)%DERIV)
          else
             reAllocate = .false.
          end if
       end if
       if (reAllocate) then
          allocate(spl(iSpln)%X(N),spl(iSpln)%Y(N), &
               &   spl(iSpln)%DERIV(N,2), stat=IERR2)
          if (IERR2 /= 0) then
             call ffamsg_list ('Error :   Allocation failure',IERR2)
             goto 915
          end if
       end if
       last(iSpln) = IKF
       spl(iSpln)%N = N
       spl(iSpln)%X = RFUNC(1,:)
       spl(iSpln)%Y = RFUNC(2,:)
       IOP = -1
    end if

    FVALUE = 0.0_dp
    call SPLIN3 (spl(iSpln)%X,spl(iSpln)%Y,spl(iSpln)%DERIV, &
         &       spl(iSpln)%N,Z,FVALUE,FDERIV,M_p,IOP,IERR2)
    if (IERR2 /= 0) goto 915

    if (calcPastEnd .and. extrapolate == 2) then
       if (T < Z(1)) then
          FVALUE(1) = FVALUE(1) + spl(iSpln)%DERIV(1,1)*(T-Z(1))
       else if (T > Z(1)) then
          FVALUE(1) = FVALUE(1) + spl(iSpln)%DERIV(spl(iSpln)%N,1)*(T-Z(1))
       end if
    end if

    FUNC6 = FVALUE(1)
    return

915 IERR = IERR - 1
    call ffamsg_list ('Error :   Could not evaluate spline function')
    call ffamsg_list ('          FUNC6, baseId =',ikf)

  end function FUNC6


  subroutine deallocateSplines ()
    integer :: i
    last = 0
    do i = 1, nSpln_p
       if (spl(i)%N > 0) then
          deallocate(spl(i)%X,spl(i)%Y,spl(i)%DERIV)
          spl(i)%N = 0
       end if
    end do
  end subroutine deallocateSplines


  !***********************************************************************
  !************************************************************  SPLIN3  *
  !***********************************************************************

  subroutine SPLIN3 (X,Y,DERIV,N,Z,FVALUE,FDERIV,M,IOP,IERR)

    !     CERN LIBRARY PROGRAM NO E-209.
    !     NUMMAT LIBRARY PROGRAM NO E201.
    !
    !     REVISED VERSION JULY 1973.
    !
    !     PURPOSE = TO COMPUTE A NATURAL SPLINE APPROXIMATION OF THIRD
    !               ORDER FOR A FUNCTION Y(X) GIVEN IN THE N POINTS
    !               (X(I),Y(I)) , I=1...N.
    !
    !     PARAMETERS (IN LIST).
    !
    !     X       = AN ARRAY STORING THE INPUT ARGUMENTS.DIMENSION X(N).
    !     Y       = AN ARRAY STORING THE INPUT FUNCTION VALUES.THE ELEMENT
    !               Y(I) REPRESENT THE FUNCTION VALUE Y(X) FOR X=X(I).
    !     DERIV   = AN ARRAY USED FOR STORING THE COMPUTED DERIVATIVES OF
    !               THE FUNCTION Y(X).IN DERIV(I,1) AND DERIV(I,2) ARE
    !               STORED THE FIRST- AND SECOND-ORDER DERIVATIVES OF Y(X)
    !               FOR X=X(I) RESPECTIVELY.
    !     N       = NUMBER OF INPUT FUNCTION VALUES.
    !     Z       = AN ARRAY STORING THE ARGUMENTS FOR THE INTERPOLATED
    !               VALUES TO BE COMPUTED.
    !     FVALUE  = AN ARRAY STORING THE COMPUTED INTERPOLATED VALUES.
    !               FVALUE(J) REPRESENT THE FUNCTION VALUE FVALUE(Z) FOR
    !               Z=Z(J).
    !     FDERIV  = AN ARRAY USED FOR STORING THE DERIVATIVES OF THE COM-
    !               PUTED INTERPOLATED VALUES.EXPLANATION AS FOR DERIV.
    !     M       = NUMBER OF INTERPOLATED VALUES TO BE COMPUTED.
    !     IOP     = OPTION PARAMETER.FOR IOP.LE.0 THE DERIVATIVES FOR EACH
    !               SUB-INTERVAL IN THE SPLINE APPROXIMATION ARE COMPUTED.
    !               <0, THE SECOND ORDER END-POINT DERIVATIVES ARE
    !                   COMPUTED BY LINEAR EXTRAPOLATION.
    !               =0, THE SECOND ORDER END-POINT DERIVATIVES ASSUMED TO
    !                   BE GIVEN.
    !               =1, COMPUTE SPLINE APPROXIMATIONS FOR THE ARGUMENTS
    !                   GIVEN IN THE ARRAY Z,THE DERIVATIVES BEEING
    !                   ASSUMED TO HAVE BEEN CALCULATED IN A PREVIOUS CALL
    !                   ON THE ROUTINE.
    !     IERR    = ERROR PARAMETER.
    !               =0, NO ERRORS OCCURRED.
    !               =1, THE NUMBER OF POINTS TOO SMALL I.E. N LESS THAN 4.
    !               =2, THE ARGUMENTS X(I) NOT IN INCREASING ORDER.
    !               =3, ARGUMENT TO BE USED IN INTERPOLATION ABOVE RANGE.
    !               =4, ARGUMENT TO BE USED IN INTERPOLATION BELOW RANGE.
    !
    !     LOCAL VARIABLES (STATIC).
    !
    !     SECD1   = VALUE OF THE SECOND DERIVATIVE D2Y(X)/DX2 FOR THE
    !               INPUT ARGUMENT X=X(1).
    !     SECDN   = VALUE OF THE SECOND DERIVATIVE D2Y(X)/DX2 FOR THE
    !               INPUT ARGUMENT X=X(N).
    !               NB. VALUES HAVE TO BE ASSIGNED TO SECD1 AND SECDN IN
    !               THE CALLING PROGRAM.IF A NATURAL SPLINE FIT IS WANTED,
    !               PUT SECD1=SECDN=0.
    !     VOFINT  = COMPUTED APPROXIMATION FOR THE INTEGRAL OF Y(X) TAKEN
    !               FROM X(1) TO X(N).
    !     NXY     = N (SEE ABOVE),HAS TO BE STORED FOR ENTRIES
    !               CORRESPONDING TO IOP=1.

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: N, M, IOP
    real(dp), intent(in)    :: X(:), Y(:), Z(:)
    real(dp), intent(inout) :: DERIV(:,:)
    real(dp), intent(out)   :: FVALUE(:), FDERIV(:,:)
    integer , intent(out)   :: IERR

    !! Local variables

    integer  :: I, J, II, IL, IP, IU
    real(dp) :: A0, A1, A2, A3, A4, A5, A6, ALF, ALF1, ALFN, ARG, BET
    real(dp) :: BET1, BETN, DIVDIF, DXINV, DXMIN, DXPLUS, DYMIN, DYPLUS
    real(dp) :: XL, XU

    character(len=128) :: errmsg

    integer , save :: NXY = 0
    real(dp), save :: SECD1, SECDN, VOFINT

    real(dp), parameter :: ZERO  = 0.0_dp
    real(dp), parameter :: HALF  = 0.5_dp
    real(dp), parameter :: ONE   = 1.0_dp
    real(dp), parameter :: THREE = 3.0_dp
    real(dp), parameter :: THIRD = 0.3333333333333333_dp
    real(dp), parameter :: SIXTH = 0.1666666666666667_dp

    !! --- Logic section ---

    IERR=0
    if (IOP.gt.0) GOTO 1110

    !     CHECK IF ENOUGH DATA-POINTS ARE AVAILABLEI.E. IF N LESS THAN 4
    !     NO THIRD ORDER SPLINE APPROXIMATION IS POSSIBLE.

    if (N.lt.4) GOTO 2010

    !     START CALCULATION OF COEFFICIENTS TO BE USED IN THE SYSTEM
    !     OF EQUATIONS FOR THE SECOND ORDER DERIVATIVES OF Y(X).

    if (IOP.eq.-1) then
       SECD1=ZERO
       SECDN=ZERO
       BET1=ONE/(ONE+HALF*(X(2)-X(1))/(X(3)-X(2)))
       ALF1=BET1*(ONE-((X(2)-X(1))/(X(3)-X(2)))**2)
       BETN=ONE/(ONE+HALF*(X(N)-X(N-1))/(X(N-1)-X(N-2)))
       ALFN=BETN*(ONE-((X(N)-X(N-1))/(X(N-1)-X(N-2)))**2)
    end if

    DERIV(1,2)=SECD1
    DERIV(N,2)=SECDN
    DERIV(1,1)=ZERO
    DXPLUS=X(2)-X(1)

    !     CHECK IF ARGUMENTS ARE IN INCREASING ORDER.IF NOT PRINT ERROR
    !     MESSAGE AND STOP.

    I=1
    if (DXPLUS.le.ZERO) GOTO 2020

    DYPLUS=(Y(2)-Y(1))/DXPLUS
    IU=N-1
    do I=2,IU
       DXMIN =DXPLUS
       DYMIN =DYPLUS
       DXPLUS=X(I+1)-X(I)

       !     CHECK IF ARGUMENTS ARE IN INCREASING ORDER.IF NOT PRINT ERROR
       !     MESSAGE AND STOP.

       if (DXPLUS.le.ZERO) GOTO 2020

       DXINV =ONE/(DXPLUS+DXMIN)
       DYPLUS=(Y(I+1)-Y(I))/DXPLUS
       DIVDIF=DXINV*(DYPLUS-DYMIN)
       ALF =HALF*DXINV*DXMIN
       BET =HALF-ALF

       if (I.eq.2) DIVDIF=DIVDIF-THIRD*ALF*DERIV(1,2)
       if (I.eq.IU) DIVDIF=DIVDIF-THIRD*BET*DERIV(N,2)
       if (I.eq.2) ALF=ZERO

       if (IOP.eq.-1) then
          if (I.eq.2) then
             BET=BET*ALF1
             DIVDIF=DIVDIF*BET1
          else if (I.eq.IU) then
             ALF=ALF*ALFN
             DIVDIF=DIVDIF*BETN
          end if
       end if
       DXINV=ONE/(ONE+ALF*DERIV(I-1,1))
       DERIV(I,1)=-DXINV*BET
       DERIV(I,2)= DXINV*(THREE*DIVDIF-ALF*DERIV(I-1,2))
    end do

    !     COMPUTE THE SECOND DERIVATIVES BY BACKWARDS RECURRENCE RELATION.
    !     THE SECOND ORDER DERIVATIVES FOR X=X(N-1) ALREADY COMPUTED.

    do I=2,IU
       J=N-I
       DERIV(J,2)=DERIV(J,1)*DERIV(J+1,2)+DERIV(J,2)
    end do

    if (IOP.eq.-1) then
       DERIV(1,2)=((X(3)-X(1))/(X(3)-X(2)))*DERIV(2,2) &
            &    -((X(2)-X(1))/(X(3)-X(2)))*DERIV(3,2)
       DERIV(N,2)=-((X(N)-X(N-1))/(X(N-1)-X(N-2)))*DERIV(N-2,2) &
            &    + ((X(N)-X(N-2))/(X(N-1)-X(N-2)))*DERIV(N-1,2)
    end if

    !     CALCULATION OF THE SECOND ORDER DERIVATIVES FINISHED. START
    !     CALCULATION OF THE FIRST ORDER DERIVATIVES AND OF THE INTEGRAL.

    VOFINT=ZERO
    do I=1,IU
       DXPLUS=X(I+1)-X(I)
       DYPLUS=Y(I+1)-Y(I)
       DIVDIF=DYPLUS/DXPLUS
       DERIV(I,1)=DIVDIF-DXPLUS*(THIRD*DERIV(I,2)+SIXTH*DERIV(I+1,2))
       DXPLUS=HALF*DXPLUS
       VOFINT=VOFINT+DXPLUS*(Y(I+1)+Y(I)-THIRD*(DERIV(I+1,2)+DERIV(I, &
            2))*DXPLUS**2)
    end do

    !     COMPUTE THE LAST FIRST ORDER DERIVATIVE.

    DXPLUS=X(N)-X(N-1)
    DYPLUS=Y(N)-Y(N-1)
    DIVDIF=DYPLUS/DXPLUS
    DERIV(N,1)=DIVDIF+DXPLUS*(SIXTH*DERIV(N-1,2)+THIRD*DERIV(N,2))

    !     CALCULATION OF FIRST ORDER DERIVATIVES AND INTEGRAL FINISHED.

    NXY=N

    !     COMPUTE INTERPOLATED VALUES IF ANY.

1110 if (M.lt.1) return

    XL=X(1)
    XU=X(2)
    IP=3
    IL=0

    do J=1,M
       ARG=Z(J)
       if (ARG.gt.XU) then

          ! ARGUMENT ABOVE PRESENT RANGE. SHIFT RANGE UPWARDS.

          do I=IP,NXY
             if (ARG.le.X(I)) then
                XL=X(I-1)
                XU=X(I)
                IP=I+1
                IL=0
                GOTO 1140
             end if
          end do

          ! ARGUMENT OUT OF RANGE, I.E. ARG GREATER THAN X(N).

          IERR=3
          IP=NXY+1
          GOTO 1090

       else if (ARG.lt.XL) then

          ! ARGUMENT BELOW PRESENT RANGE. SHIFT DOWNWARDS.

          do I=1,IP
             II=IP-I-2
             if (II.eq.0) exit
             if (ARG.ge.X(II)) then
                XL=X(II)
                XU=X(II+1)
                IP=II+2
                IL=0
                GOTO 1140
             end if
          end do

          ! ARGUMENT OUT OF RANGE, I.E. ARG LESS THAN X(1).

          IERR=4
          IP=3
          GOTO 1090

       end if

       !     ARGUMENT IN CORRECT RANGE.CHECK IF POLYNOMIAL COEFFICIENTS
       !     HAVE TO BE CALCULATED.

1140   if (IL.le.0) then

          ! COMPUTE POLYNOMIAL COEFFICIENTS.

          II=IP-2
          A0=Y(II)
          A1=DERIV(II,1)
          A4=DERIV(II,2)
          A6=(DERIV(II+1,2)-A4)/(XU-XL)
          A2=HALF*A4
          A3=SIXTH*A6
          A5=HALF*A6
          IL=1

       end if

       !     CALCULATION OF POLYNOMIAL COEFFICIENTS FINISHED.
       !     COMPUTE VALUES.

       ARG=ARG-XL
       FVALUE(J)=((A3*ARG+A2)*ARG+A1)*ARG+A0
       FDERIV(J,1)=(A5*ARG+A4)*ARG+A1
       FDERIV(J,2)=A6*ARG+A4
       cycle

1090   write(errmsg,3000) IERR, ARG
       call ffamsg_list (trim(errmsg))

       FVALUE(J)=ZERO
       FDERIV(J,1)=ZERO
       FDERIV(J,2)=ZERO

       II=IP-2
       XL=X(II)
       XU=X(II+1)
       IL=0

    end do

    !     CALCULATION OF INTERPOLATED VALUES FINISHED.

    return

    !     PRINT ERROR MESSAGES.

2000 call ffamsg_list (trim(errmsg))
    return
2010 IERR=1
    write(errmsg,3000) IERR
    goto 2000
2020 IERR=2
    write(errmsg,3000) IERR, X(I), X(I+1)
    goto 2000

3000 format('Error :   subroutine SPLIN3 ERROR NO',I3,' ***',1P2E25.14)

  end subroutine SPLIN3


  !***********************************************************************
  !************************************************************  FUNC 8  *
  !***********************************************************************

  function FUNC8 (RFUNC,T,X,IERR) result(F)

    !!==========================================================================
    !! Evaluates the streamline wave profile at point X and time T.
    !!
    !! RFUNC  - Wave data, output from initFUNC8
    !! x      - Current position
    !! t      - Current time
    !! f      - Current wave height above z=0
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 09 May 2011 / 1.0
    !!==========================================================================

    real(dp), intent(in)    :: RFUNC(:), T, X
    integer , intent(inout) :: IERR

    !! Local variables
    integer  :: i, N
    real(dp) :: F, theta

    !! --- Logic section ---

    F = 0.0_dp
    N = (size(RFUNC)-3)/2
    if (n < 0) IERR = IERR + n

    theta = RFUNC(3) + RFUNC(1)*x - RFUNC(2)*t

    do i = 1, n
       f = f + RFUNC(3+i)*cos(real(i,dp)*theta)
    end do

  end function FUNC8


  !***********************************************************************
  !************************************************************  FUNC 9  *
  !***********************************************************************

  function FUNC9 (IFUNC,RFUNC,T,X,IERR) result(F)

    !!==========================================================================
    !! Evaluates the embedded wave profile at point X and time T.
    !!
    !! RFUNC  - Wave data, output from initFUNC9
    !! x      - Current position
    !! t      - Current time
    !! f      - Current wave height above z=0
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 14 May 2012 / 1.0
    !!==========================================================================

    integer , intent(in)    :: IFUNC(:)
    real(dp), intent(in)    :: RFUNC(:), T, X
    integer , intent(inout) :: IERR

    !! Local variables
    logical  :: irrW
    integer  :: i, N, M, ipSt, nRFL
    real(dp) :: F, FS, Ts, Tp, cdt, scS, scL, startBl, stopBl

    !! --- Logic section ---

    N = IFUNC(3)
    M = IFUNC(4)
    if (M == 3) then
       nRFL = 3*N + 1
    else
       nRFL = M*N
    end if
    ipSt = nRFL + 3
    stopBl = RFUNC(ipSt-2)
    startBl = RFUNC(ipSt-1)

    !! Check if we are within any of the streamline wave function domains
    Ts = 0.0_dp
    Tp = 0.0_dp
    irrW = .true.
    do i = 5, size(IFUNC)
       n = 6 + 2*IFUNC(i)
       if (ipSt+1 <= size(RFUNC)) then
          Ts = T-RFUNC(ipSt)
          Tp = RFUNC(ipSt+1)
          if (abs(Ts) <= startBl*Tp) then
             irrW = abs(Ts) > stopBl*Tp
             exit
          end if
       end if
       ipSt = ipSt + n
    end do

    if (irrW) then
       !! Evaluate the irregular wave
       F = FUNC4(IFUNC,RFUNC(1:nRFL),T,X,IERR)
    else
       !! Evaluate the streamline wave function (no blending)
       F = FUNC8(RFUNC(ipSt+3:ipSt+n-1),Ts,X,IERR)
    end if
    if (.not. irrW .or. i > size(IFUNC)) return

    !! We are in the transition zone where the two wave formulations
    !! are to be blended, evaluate the smooth transfer functions
    cdt = cos(pi*(abs(Ts)/Tp-stopBl)/(startBl-stopBl))
    scS = 0.5_dp*(1.0_dp+cdt)
    scL = 0.5_dp*(1.0_dp-cdt)

    !! Evaluate the streamline wave function to blend with the irregular wave
    FS = FUNC8(RFUNC(ipSt+3:ipSt+n-1),Ts,X,IERR)

    !! Compute the blended wave height
    F = F*scL + FS*scS

  end function FUNC9


  !***********************************************************************
  !************************************************************  FUNC10  *
  !***********************************************************************

  function FUNC10 (IKF,IFUNC,RFUNC,X,IERR) result(FV)

    !!==========================================================================
    !! Evaluates a piece-wise linear function at the given point.
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 17 Nov 2014 / 2.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: IKF, IFUNC(:)
    real(dp), intent(in)    :: RFUNC(:,:), X
    integer , intent(inout) :: IERR

    !! Local variables
    integer  :: I, N, extrapolate
    real(dp) :: X0, X1, Y, F0, F1, FV

    !! --- Logic section ---

    N = size(RFUNC,2)
    if (N < 2) then
       if (N == 1) then
          FV = RFUNC(2,1)
       else
          FV = 0.0_dp
       end if
       return
    end if

    extrapolate = IFUNC(2)

    I  = 0
    X0 = RFUNC(1,1)
    F0 = RFUNC(2,1)
    X1 = RFUNC(1,N)
    F1 = RFUNC(2,N)

    if (X > X0 .and. X < X1) then

       !! Find the interval to evaluate the linear function from
       X1 = X0
       F1 = F0
       do I = 2, N
          Y = RFUNC(1,I)
          if (Y > X) then
             X1 = Y
             F1 = RFUNC(2,I)
             exit
          else if (Y >= X0) then
             X0 = Y
             F0 = RFUNC(2,I)
          end if
       end do

    else if (X < X0 .and. extrapolate > 0) then

       !! Extrapolation before function start
       if (extrapolate == 1) then ! flat
          FV = RFUNC(2,1)
          return
       else if (extrapolate == 2) then ! linear
          I  = 1
          X1 = RFUNC(1,2)
          F1 = RFUNC(2,2)
       end if

    else if (X > X1 .and. extrapolate > 0) then

       !! Extrapolation past function end
       if (extrapolate == 1) then ! flat
          FV = RFUNC(2,N)
          return
       else if (extrapolate == 2) then ! linear
          I  = N
          X0 = RFUNC(1,N-1)
          F0 = RFUNC(2,N-1)
       end if

    else if (X <= X0 .and. X0-X < epsArg_p) then

       !! TT #2696: Avoid out-of-range error due to round-off errors, etc.
       FV = F0
       return

    else if (X >= X1 .and. X-X1 < epsArg_p) then

       !! TT #2696: Avoid out-of-range error due to round-off errors, etc.
       FV = F1
       return

    else

       !! Function argument is out-of range
       IERR = IERR-15
       call outOfRangeError (X,X0,X1)
       call ffamsg_list ('          FUNC10, baseId =',ikf)
       FV = 0.0_dp
       return

    end if

    if (X1-X0 > epsArg_p) then
       FV = F0 + (X-X0)*(F1-F0)/(X1-X0)
    else if (I == 1) then
       FV = F0
    else if (I == N) then
       FV = F1
    else
       FV = 0.5_dp*(F0+F1) ! The unlikely event X0 < X < X1 and X1-X0 < epsArg_p
    end if

  end function FUNC10


  function FUNC10diff (IKF,IFUNC,RFUNC,X,IERR) result(FV)

    !!==========================================================================
    !! Evaluates the derivative of a piece-wise linear function.
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 17 Nov 2014 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: IKF, IFUNC(:)
    real(dp), intent(in)    :: RFUNC(:,:), X
    integer , intent(inout) :: IERR

    !! Local variables
    integer  :: I, N, extrapolate
    real(dp) :: X0, X1, Y, F0, F1, FV

    character(len=64) :: errMsg

    !! --- Logic section ---

    FV = 0.0_dp
    N  = size(RFUNC,2)
    if (N < 2) return

    extrapolate = IFUNC(2)

    X0 = RFUNC(1,1)
    F0 = RFUNC(2,1)
    X1 = RFUNC(1,N)
    F1 = RFUNC(2,N)

    if (X > X0 .and. X < X1) then

       !! Find the interval to evaluate the linear function from
       X1 = X0
       F1 = F0
       do I = 2, N
          Y = RFUNC(1,I)
          if (Y > X) then
             X1 = Y
             F1 = RFUNC(2,I)
             exit
          else if (Y >= X0) then
             X0 = Y
             F0 = RFUNC(2,I)
          end if
       end do

    else if (X <= X0) then

       !! Extrapolation before function start
       if (extrapolate == 1) then ! flat
          return
       else if (extrapolate == 2 .or. X+epsArg_p > X0) then ! linear
          X1 = RFUNC(1,2)
          F1 = RFUNC(2,2)
       else
          !! Function argument is out-of range
          IERR = IERR-15
          call outOfRangeError (X,X0,X1)
          call ffamsg_list ('          FUNC10diff, baseId =',ikf)
          return
       end if

    else if (X >= X1) then

       !! Extrapolation past function end
       if (extrapolate == 1) then ! flat
          return
       else if (extrapolate == 2 .or. X-epsArg_p < X1) then ! linear
          X0 = RFUNC(1,N-1)
          F0 = RFUNC(2,N-1)
       else
          !! Function argument is out-of range
          IERR = IERR-15
          call outOfRangeError (X,X0,X1)
          call ffamsg_list ('          FUNC10diff, baseId =',ikf)
          return
       end if

    end if

    if (X1-X0 > epsArg_p) then
       FV = (F1-F0)/(X1-X0)
    else
       IERR = IERR-15
       write(errMsg,900) X
900    format('Evaluating derivative at a discontinuity, x =',1PE13.5)
       call ffamsg_list ('Error :   '//errMsg)
       call ffamsg_list ('          FUNC10diff, baseId =',ikf)
    end if

  end function FUNC10diff


  function FUNC10integral (IKF,IFUNC,RFUNC,X,IORD,IERR)

    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : Integrate a piece-wise linear function
    !  *                    from 0.0 to X
    !  *
    !  * ENTRY ARGUMENTS  : Integration order (0, 1, or 2)..........IORD
    !  *
    !  * EXIT ARGUMENTS   : Function integral at point X...............F
    !  *                    Error indicator.........................IERR
    !  *
    !  * ERROR INDICATION : IERR < 0  -->  Error has occurred
    !  *
    !  * AUTHOR           : Bjorn Haugen 1998
    !***********************************************************************

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: IKF, IFUNC(:), IORD
    real(dp), intent(in)    :: RFUNC(:,:), X
    integer , intent(inout) :: IERR
    real(dp)                :: FUNC10integral

    !! Local variables
    integer  :: i1, i2, inc, isize, extrapolate
    real(dp) :: x1, x2, k1, k2, f1, f2, e2, dx

    real(dp), parameter :: EPS = 1.0D-12

    !! --- Logic section ---

    func10integral = 0.0_dp
    if (abs(X) < EPS .and. IORD > 0) return
    if (size(rfunc,2) < 1) return

    extrapolate = ifunc(2)

    ! --- Find starting point; i.e. point below zero (i1) and above zero (i2)
    ! --- Also find the total number of points in the curve definition (isize)

    i1 = 0
    i2 = 0
    isize = 1
    !kmo, 28.10.02: Changed the "<" to "<=" such that the do-while loop
    !               is not terminated on equal x-values in the rfunc array.
    do while (isize < size(rfunc,2))
       if (rfunc(1,isize) > rfunc(1,isize+1)) then
          exit
       else if (rfunc(1,isize) <= 0.0_dp .and. rfunc(1,isize+1) >= 0.0_dp) then
          i1 = isize
          i2 = isize + 1
       end if
       isize = isize + 1
    end do

    if (extrapolate > 0) then
       if (rfunc(1,1) > 0.0_dp) then
          i1 = 0
          i2 = 1
       else if (rfunc(1,isize) < 0.0_dp) then
          i1 = isize
          i2 = isize + 1
       end if
    else if (rfunc(1,1) > 0.0_dp .or. rfunc(1,isize) < 0.0_dp) then
       ierr = ierr - 1
       call ffamsg_list ('Error :   Invalid X-range in function definition')
       call ffamsg_list ('          FUNC10integral, baseId =',ikf)
       return
    else if (X < rfunc(1,1) .or. X > rfunc(1,isize)) then
       ierr = ierr - 1
       call outOfRangeError (X,rfunc(1,1),rfunc(1,isize))
       call ffamsg_list ('          FUNC10integral, baseId =',ikf)
       return
    end if

    ! --- Initialize the starting conditions for the integration loop

    if (i1 == 0) then ! Extrapolate before function start
       k2 = rfunc(2,1)
       if (extrapolate == 2 .and. isize > 1) then
          dx = rfunc(1,2) - rfunc(1,1)
          if (dx >= EPS) then
             k2 = k2 - rfunc(1,1)*(rfunc(2,2)-rfunc(2,1))/dx
          end if
       end if
    else if (i2 > isize) then ! Extrapolate past function end
       k2 = rfunc(2,isize)
       if (extrapolate == 2 .and. isize > 1) then
          dx = rfunc(1,isize) - rfunc(1,isize-1)
          if (dx >= EPS) then
             k2 = k2 - rfunc(1,isize)*(rfunc(2,isize)-rfunc(2,isize-1))/dx
          end if
       end if
    else ! Interpolate between i1 and i2
       k2 = rfunc(2,i1)
       dx = rfunc(1,i2) - rfunc(1,i1)
       if (dx >= EPS) then
          k2 = k2 - rfunc(1,i1)*(rfunc(2,i2)-rfunc(2,i1))/dx
       end if
    end if

    if (X > 0.0_dp) then
       inc = 1
       i2  = i1
    else
       inc = -1
    end if

    x2 = 0.0_dp
    f2 = 0.0_dp
    e2 = 0.0_dp

    ! --- Perform the integration in a DO-WHILE loop

    do while (abs(X-x2) > EPS)
       x1 = x2
       k1 = k2
       f1 = f2
       i2 = i2 + inc

       if (i2 < 1 .and. extrapolate > 0) then
          ! Extrapolate before function start
          x2 = X
          k2 = rfunc(2,1)
          if (extrapolate == 2 .and. isize > 1) then
             dx = rfunc(1,2) - rfunc(1,1)
             if (dx >= EPS) then
                k2 = k2 + (x2-rfunc(1,1))*(rfunc(2,2)-rfunc(2,1))/dx
             end if
          end if
       else if (i2 > isize .and. extrapolate > 0) then
          ! Extrapolate past function end
          x2 = X
          k2 = rfunc(2,isize)
          if (extrapolate == 2 .and. isize > 1) then
             i1 = isize-1
             dx = rfunc(1,isize) - rfunc(1,i1)
             if (dx >= EPS) then
                k2 = k2 + (x2-rfunc(1,isize))*(rfunc(2,isize)-rfunc(2,i1))/dx
             end if
          end if
       else if (i2 < 1 .or. i2 > isize) then
          ierr = ierr - 1
          call ffamsg_list ('Error :   Invalid function definition')
          call ffamsg_list ('          FUNC10integral, baseId =',ikf)
          return
       else if (abs(rfunc(1,i2)) <= abs(X)) then
          x2 = rfunc(1,i2)
          k2 = rfunc(2,i2)
       else
          ! Interpolate between x1 and i2
          x2 = X
          dx = rfunc(1,i2) - x1
          if (abs(dx) >= EPS) then
             k2 = k1 + (rfunc(2,i2)-k1)*(x2-x1)/dx
          else
             k2 = rfunc(2,i2)
          end if
       end if

       dx = x2 - x1
       f2 = f1 + (k1+k2)*dx/2.0_dp
       e2 = e2 + (f1+f2)*dx/2.0_dp + (k1-k2)*dx*dx/12.0_dp
    end do

    if (IORD < 1) then
       FUNC10integral = k2 ! Zero'th order integral = the function value itself
    else if (IORD == 1) then
       FUNC10integral = f2 ! First order integral
    else
       FUNC10integral = e2 ! Second order integral
    end if

  end function FUNC10integral


  !***********************************************************************
  !************************************************************ FUNC11   *
  !***********************************************************************

  function FUNC11 (RFUNC,T)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 1 1
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a ramp funtion
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Offset ...............................OFFSET
    !  *                    Ramp slope ............................SLOPE
    !  *                    Start time for ramp....................STIME
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC11(RFUNC,T)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Torleif Iversen/(OIS)  1989
    !  *                    SINTEF avd. for reguleringsteknikk
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(3), T
    real(dp)                :: FUNC11

    !! Local variables
    real(dp) :: OFFSET, SLOPE, STIME

    !! --- Logic section ---

    OFFSET = RFUNC(1)
    SLOPE  = RFUNC(2)
    STIME  = RFUNC(3)

    if (T <= STIME) then
       FUNC11 = OFFSET
    else
       FUNC11 = OFFSET + SLOPE*(T-STIME)
    end if

  end function FUNC11


  function RampIntegral (IORD,X,OFFSET,SLOPE,X0,X3)

    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : Integrate a ramp function from 0.0 to X
    !  *
    !  * ENTRY ARGUMENTS  : Integration order (1 or 2)..............IORD
    !  *                    End point of integration ..................X
    !  *                    Offset ...............................OFFSET
    !  *                    Ramp slope ............................SLOPE
    !  *                    Start point for ramp......................X0
    !  *                    End point for ramp........................X3
    !  *
    !  * EXIT ARGUMENTS   : Function integral at point X...............F
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * AUTHOR           : Knut Morten Okstad 2006
    !***********************************************************************

    integer , intent(in)           :: IORD
    real(dp), intent(in)           :: X, OFFSET, SLOPE, X0
    real(dp), intent(in), optional :: X3
    real(dp)                       :: RampIntegral

    !! Local variables
    real(dp) :: START, STOPT, X1, X2, F1, F2, F3

    !! --- Logic section ---

    F1 = 0.0_dp
    F2 = 0.0_dp
    F3 = 0.0_dp
    X1 = 0.0_dp
    if (X < 0.0_dp) X1 = -X

    START = X0 + X1
    if (present(X3)) then
       STOPT = X3 + X1
    else
       STOPT = huge(1.0_dp)
    end if

    if (X > 0.0_dp) X1 = X

    X2 = min(X1,START)
    if (X2 > 0.0_dp) then

       !! Integrate the constant section before the slope
       if (IORD == 1) then
          F1 = OFFSET * X2
       else if (IORD == 2) then
          F1 = OFFSET * X2*X2 / 2.0_dp
       else
          F1 = OFFSET
       end if

    end if

    X2 = min(X1,STOPT) - max(0.0_dp,START)
    if (X2 > 0.0_dp) then

       !! Integrate the sloped section
       if (IORD == 1) then
          if (START > 0.0_dp) then
             F2 = OFFSET*X2 + OFFSET*START   + SLOPE*X2*X2/2.0_dp
          else
             F2 = OFFSET*X2 - SLOPE*X2*START + SLOPE*X2*X2/2.0_dp
          end if
       else if (IORD == 2) then
          if (START > 0.0_dp) then
             F2 = (OFFSET*START + (OFFSET + SLOPE*X2/3.0_dp) * X2/2.0_dp) * X2
          else
             F2 = (OFFSET + (OFFSET - SLOPE*START + SLOPE*X2/3.0_dp) * X2/2.0_dp) * X2
          end if
       else
          F2 = SLOPE*X2
       end if

    end if

    X2 = X1 - STOPT
    if (X2 > 0.0_dp) then

       !! Integrate the constant section after the slope
       if (IORD == 1) then
          F3 = (OFFSET + SLOPE*(STOPT-START)) * X2
       else if (IORD == 2) then
          F3 = (OFFSET + SLOPE*(STOPT-START)) * X2*X2 / 2.0_dp
       else
          F3 = -SLOPE*X2
       end if

    end if

    if (X < 0.0_dp .and. IORD == 1) then
       RampIntegral = -F1 - F2 - F3
    else
       RampIntegral =  F1 + F2 + F3
    end if

  end function RampIntegral


  !***********************************************************************
  !************************************************************  FUNC12  *
  !***********************************************************************

  function FUNC12 (RFUNC,T)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 1 2
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a step funtion
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Offset ...............................OFFSET
    !  *                    Step amplitude...........................AMP
    !  *                    Start time for ramp....................STIME
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC12(RFUNC,T)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Torleif Iversen/(OIS) 1989
    !  *                    SINTEF avd. for maskinkonstruksjon
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(3), T
    real(dp)                :: FUNC12

    !! Local variables
    real(dp) :: OFFSET, AMP, STIME

    !! --- Logic section ---

    OFFSET = RFUNC(1)
    AMP    = RFUNC(2)
    STIME  = RFUNC(3)

    if (T <= STIME) then
       FUNC12 = OFFSET
    else
       FUNC12 = OFFSET + AMP
    end if

  end function FUNC12


  !***********************************************************************
  !************************************************************  FUNC13  *
  !***********************************************************************

  function FUNC13 (RFUNC,T)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 1 3
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a square puls function
    !  *
    !  * ENTRY ARGUMENTS  : Time ......................................T
    !  *                    Offset ...............................OFFSET
    !  *                    Step amplitude ..........................AMP
    !  *                    Period ...............................PERIOD
    !  *                    Phrase angle .........................PANGLE
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC13(RFUNC,T)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Torleif Iversen/(OIS) 1989
    !  *                    SINTEF avd. for reguleringsteknikk
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(4), T
    real(dp)                :: FUNC13

    !! Local variables
    real(dp) :: OFFSET, AMP, PERIOD, PANGLE

    !! --- Logic section ---

    OFFSET = RFUNC(1)
    AMP    = RFUNC(2)
    PERIOD = RFUNC(3)
    PANGLE = RFUNC(4)

    if (DMOD(T+PANGLE,PERIOD) < 0.5_dp*PERIOD) then
       FUNC13 = OFFSET + AMP
    else
       FUNC13 = OFFSET - AMP
    end if

  end function FUNC13


  !***********************************************************************
  !************************************************************  FUNC14  *
  !***********************************************************************

  function FUNC14 (RFUNC,T)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 1 4
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a dirac puls function
    !  *
    !  * ENTRY ARGUMENTS  : Time ......................................T
    !  *                    Offset ...............................OFFSET
    !  *                    Step amplitude ..........................AMP
    !  *                    Start time for puls....................STIME
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC14(RFUNC,T)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Torleif Iversen/(OIS) 1989
    !  *                    SINTEF avd. for reguleringsteknikk
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(4), T
    real(dp)                :: FUNC14

    !! Local variables
    real(dp) :: OFFSET, AMP, DELTA, STIME

    !! --- Logic section ---

    OFFSET = RFUNC(1)
    AMP    = RFUNC(2)
    DELTA  = RFUNC(3)
    STIME  = RFUNC(4)

    if (STIME >= T-0.5_dp*DELTA .and. STIME < T+0.5_dp*DELTA) then
       FUNC14 = OFFSET + AMP
    else
       FUNC14 = OFFSET
    end if

  end function FUNC14


  !***********************************************************************
  !************************************************************  FUNC15  *
  !***********************************************************************

  function FUNC15 (RFUNC)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 1 5
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a constant function
    !  *
    !  * ENTRY ARGUMENTS  : Constant.............................. CONST
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC15(RFUNC)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Torleif Iversen/(OIS) 1989
    !  *                    SINTEF avd. for reguleringsteknikk
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(1)
    real(dp)                :: FUNC15

    !! --- Logic section ---

    FUNC15 = RFUNC(1)

  end function FUNC15


  !***********************************************************************
  !************************************************************ FUNC16   *
  !***********************************************************************

  function FUNC16 (RFUNC,T)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 1 6
    !  *
    !***********************************************************************
    !  *
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe an extended ramp funtion
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Offset ...............................OFFSET
    !  *                    Ramp slope ............................SLOPE
    !  *                    Start time for ramp....................STIME
    !  *                    Stop time for ramp.....................STOPT
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC16(RFUNC,T)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Terje Rolvag/(OIS)  1989
    !  *                    SINTEF avd. for reguleringsteknikk
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(4), T
    real(dp)                :: FUNC16

    !! Local variables
    real(dp) :: OFFSET, SLOPE, STIME, STOPT

    !! --- Logic section ---

    OFFSET = RFUNC(1)
    SLOPE  = RFUNC(2)
    STIME  = RFUNC(3)
    STOPT  = RFUNC(4)

    if (T <= STIME) then
       FUNC16 = OFFSET
    else if (T < STOPT) then
       FUNC16 = OFFSET + SLOPE*(T-STIME)
    else
       FUNC16 = OFFSET + SLOPE*(STOPT-STIME)
    end if

  end function FUNC16


  !***********************************************************************
  !************************************************************  FUNC17  *
  !***********************************************************************

  function FUNC17 (RFUNC,T)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 1 7
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a scaling function
    !  *
    !  * ENTRY ARGUMENTS  : Time.......................................T
    !  *                    Scale Factor.......................... SCALE
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T................F
    !  *
    !  * CALLING SEQUENCE : FN=FUNC17(RFUNC,T)
    !  *
    !  * ERROR INDICATION : None
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Jens Lien
    !  *                    Fedem AS
    !***********************************************************************

    real(dp), intent(in)    :: RFUNC(1), T
    real(dp)                :: FUNC17

    !! --- Logic section ---

    FUNC17 = RFUNC(1)*T

  end function FUNC17


  !***********************************************************************
  !************************************************************  FUNC18  *
  !***********************************************************************

  function func18 (ikf,iFunc,rFunc,arg,ierr)

    !!==========================================================================
    !! Calls a C-function to read and interpolate a function value from a file.
    !!
    !! Programmer : Tommy Jorstad / Knut Morten Okstad
    !! date/rev   : June 2002 / 2.0
    !!==========================================================================

    use FFaDeviceFunctionInterface, only : FiDF_GetValue
    use FFaMsgInterface           , only : ffamsg_list

    integer , intent(in)    :: ikf, iFunc(5)
    real(dp), intent(in)    :: rFunc(2), arg
    integer , intent(inout) :: ierr
    real(dp)                :: func18

    !! Local variables
    integer :: fileId, err

    !! --- Logic section ---

    err    = 0
    fileId = iFunc(3) !     fileId         channel  zeroAdj  shift    scale
    func18 = FiDF_GetValue (fileId,arg,err,iFunc(4),iFunc(5),rFunc(1),rFunc(2))
    if (err == 0) return

    ierr = ierr - 1
    call ffamsg_list ('Error :   Could not read from external device:',fileId)
    call ffamsg_list ('          FUNC18, baseId =',ikf)

  end function func18


  function func18integral (ikf,iFunc,rFunc,arg,iorder,ierr) result(fInt18)

    !!==========================================================================
    !! Calls a C-function to read and integrate a function from a file.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : December 2004 / 1.0
    !!==========================================================================

    use FFaDeviceFunctionInterface, only : FiDF_GetValue
    use FFaMsgInterface           , only : ffamsg_list

    integer , intent(in)    :: ikf, iFunc(5), iorder
    real(dp), intent(in)    :: rFunc(2), arg
    integer , intent(inout) :: ierr

    !! Local variables
    integer  :: fileId, err
    real(dp) :: fInt18

    !! --- Logic section ---

    err    = iorder
    fileId = iFunc(3) !     fileId         channel  zeroAdj  shift    scale
    fInt18 = FiDF_GetValue (fileId,arg,err,iFunc(4),iFunc(5),rFunc(1),rFunc(2))
    if (err >= 0) return

    ierr = ierr - 1
    call ffamsg_list ('Error :   Could not read from external device:',fileId)
    call ffamsg_list ('          FUNC18integral, baseId =',ikf)

  end function func18integral

  !***********************************************************************
  !************************************************************  FUNC19  *
  !***********************************************************************

  function func19 (ikf,iFunc,arg,ierr)

    !!==========================================================================
    !! Calls a C-function to get function value from a user-defined expression.
    !!
    !! Programmer : Tommy Jorstad
    !! date/rev   : August 2002 / 1.0
    !!==========================================================================

    use FFaMathExprInterface, only : ffame_getValue
    use FFaMsgInterface     , only : ffamsg_list

    integer , intent(in)    :: ikf, iFunc(3)
    real(dp), intent(in)    :: arg
    integer , intent(inout) :: ierr
    real(dp)                :: func19

    !! Local variables
    integer :: exprId, err

    !! --- Logic section ---

    exprId = iFunc(3)
    func19 = ffame_getValue (exprId,arg,err)
    if (err == 0) return

    ierr = ierr - 1
    call ffamsg_list ('Error :   Could not get value from expression',exprId)
    call ffamsg_list ('          FUNC19, baseId =',ikf)

  end function func19

  !***********************************************************************
  !********************************************************  FUNC19diff  *
  !***********************************************************************

  function func19diff (ikf,iFunc,arg,ierr)

    !!==========================================================================
    !! Calls a C-function to get function derivative from a defined expression.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : September 2009 / 1.0
    !!==========================================================================

    use FFaMathExprInterface, only : ffame_getDiff
    use FFaMsgInterface     , only : ffamsg_list

    integer , intent(in)    :: ikf, iFunc(3)
    real(dp), intent(in)    :: arg
    integer , intent(inout) :: ierr
    real(dp)                :: func19diff

    !! Local variables
    integer :: exprId, err

    !! --- Logic section ---

    exprId = iFunc(3)
    func19diff = ffame_getDiff (exprId,arg,err)
    if (err == 0) return

    ierr = ierr - 1
    call ffamsg_list ('Error :   Could not differentiate expression',exprId)
    call ffamsg_list ('          FUNC19diff, baseId =',ikf)

  end function func19diff

  !***********************************************************************
  !************************************************************  FUNC20  *
  !***********************************************************************

  function func20 (ikf,iFunc,args,ierr)

    !!==========================================================================
    !! Calls a C-function to get function value from a user-defined expression.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : September 2009 / 1.0
    !!==========================================================================

    use FFaMathExprInterface, only : ffame_getValue2
    use FFaMsgInterface     , only : ffamsg_list

    integer , intent(in)    :: ikf, iFunc(4)
    real(dp), intent(in)    :: args(:)
    integer , intent(inout) :: ierr
    real(dp)                :: func20

    !! Local variables
    integer :: exprId, err

    !! --- Logic section ---

    exprId = iFunc(3)
    func20 = ffame_getValue2 (exprId,args,err)
    if (err == 0) return

    ierr = ierr - 1
    call ffamsg_list ('Error :   Could not get value from expression',exprId)
    call ffamsg_list ('          FUNC20, baseId =',ikf)

  end function func20

  !***********************************************************************
  !********************************************************  FUNC20diff  *
  !***********************************************************************

  function func20diff (ikf,iFunc,args,ierr)

    !!==========================================================================
    !! Calls a C-function to get function derivative from a defined expression.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : September 2009 / 1.0
    !!==========================================================================

    use FFaMathExprInterface, only : ffame_getDiff2
    use FFaMsgInterface     , only : ffamsg_list

    integer , intent(in)    :: ikf, iFunc(4)
    real(dp), intent(in)    :: args(:)
    integer , intent(inout) :: ierr
    real(dp)                :: func20diff

    !! Local variables
    integer :: exprId, derIdx, err

    !! --- Logic section ---

    exprId = iFunc(3)
    derIdx = iFunc(4)
    func20diff = ffame_getDiff2 (exprId,derIdx,args,err)
    if (err == 0) return

    ierr = ierr - 1
    call ffamsg_list ('Error :   Could not differentiate expression',exprId)
    call ffamsg_list ('          FUNC20diff, baseId =',ikf)

  end function func20diff

  !***********************************************************************
  !************************************************************  FUNC21  *
  !***********************************************************************

  function func21 (ikf,iFunc,rFunc,args,ierr)

    !!==========================================================================
    !! Calls a C-function to get function value from a user-defined function.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : January 2010 / 1.0
    !!==========================================================================

    use FFaUserFuncInterface, only : ffauf_getValue
    use FFaMsgInterface     , only : ffamsg_list

    integer , intent(in)    :: ikf, iFunc(4)
    real(dp), intent(in)    :: rFunc(:), args(:)
    integer , intent(inout) :: ierr
    real(dp)                :: func21

    !! Local variables
    integer :: funcId, err

    !! --- Logic section ---

    funcId = iFunc(3)
    func21 = ffauf_getValue (ikf,funcId,rFunc,args,err)
    if (err == 0) return

    if (err > 0) then
       if (ierr == 0) ierr = err
       return
    endif

    ierr = ierr - 1
    call ffamsg_list ('Error :   Could not get value from user function',funcId)
    call ffamsg_list ('          FUNC21, baseId =',ikf)

  end function func21


  !***********************************************************************
  !********************************************************  FUNC21diff  *
  !***********************************************************************

  function func21diff (ikf,iFunc,rFunc,args,ierr)

    !!==========================================================================
    !! Calls a C-function to get derivative from a user-defined function.
    !!
    !! Programmer : Knut Morten Okstad
    !! date/rev   : January 2010 / 1.0
    !!==========================================================================

    use FFaUserFuncInterface, only : ffauf_getDiff
    use FFaMsgInterface     , only : ffamsg_list

    integer , intent(in)    :: ikf, iFunc(4)
    real(dp), intent(in)    :: rFunc(:), args(:)
    integer , intent(inout) :: ierr
    real(dp)                :: func21diff

    !! Local variables
    integer :: funcId, derIdx, err

    !! --- Logic section ---

    funcId = iFunc(3)
    derIdx = iFunc(4)
    func21diff = ffauf_getDiff (ikf,funcId,derIdx,rFunc,args,err)
    if (err == 0) return

    if (err > 0) then
       if (ierr == 0) ierr = err
       return
    endif

    ierr = ierr - 1
    call ffamsg_list ('Error :   Could not differentiate user function',funcId)
    call ffamsg_list ('          FUNC21diff, baseId =',ikf)

  end function func21diff


  !#######################################################################
  !#######################################################   FUNC22   ####
  !#######################################################################

  function FUNC22 (IKF,RFUNC,T,IERR) result(FN)

    !***********************************************************************
    !  *
    !  * MODUL            : F U N C 2 2
    !  *
    !***********************************************************************
    !  *
    !  * PROGRAM CATEGORY : Fortran function
    !  *
    !  * PURPOSE          : To describe a trajectory with continuity in
    !  *                    speed, acceleration and jerk
    !  *                    The trajectiory is based on a second order
    !  *                    sin function
    !  *
    !  * ENTRY ARGUMENTS  : Time.........................................T
    !  *                    Starting time for trajection generation....RF1
    !  *                    Total trajectory time......................RF2
    !  *                    Maximum acceleration.......................RF3
    !  *                    Maximum speed..............................RF4
    !  *
    !  * EXIT ARGUMENTS   : Value of function at time T.................FN
    !  *                    Error indicator...........................IERR
    !  *
    !  * CALLING SEQUENCE : FN=FUNC22(IKF,RFUNC,T,IERR)
    !  *
    !  * ERROR INDICATION : IERR < 0  --> Error has occurred
    !  *                     -10 : Zero or negative maximum acceleration
    !  *                     -15 : Zero or negative maximum speed
    !  *                     -20 : Can not reach maximum speed
    !  *                           before deacceleration
    !  *
    !  * EXTERNAL CALLS   : None
    !  *
    !  * CALLED BY        : TFUNC
    !  *
    !  * AUTHOR           : Hans Petter Hildre 503914
    !  *                    NTH Institutt for maskinkonstruksjon
    !  *
    !  * DATE/VERSION     : 90-03-27, Version 1.0
    !***********************************************************************

    use FFaMsgInterface, only : ffamsg_list

    integer , intent(in)    :: IKF
    real(dp), intent(in)    :: RFUNC(4), T
    integer , intent(inout) :: IERR

    !! Local variables
    real(dp) :: RF1, RF2, RF3, RF4, TF, W, FN

    !! --- Logic section ---

    FN  = 0.0_dp
    RF1 = RFUNC(1)
    RF2 = RFUNC(2)
    RF3 = RFUNC(3)
    RF4 = RFUNC(4)

    if (RF3 < epsArg_p) then

       IERR = IERR - 10
       call ffamsg_list ('Error :   Zero or negative maximum acceleration')
       GOTO 915

    else if (RF4 < epsArg_p) then

       IERR = IERR - 15
       call ffamsg_list ('Error :   Zero or negative maximum speed')
       GOTO 915

    else if (RF2*RF3 < 4.0_dp*RF4) then

       IERR = IERR - 20
       call ffamsg_list ('Error :   Can not reach maximum speed'// &
            &            ' before deacceleration')
       GOTO 915

    end if

    TF = T - RF1          ! function time
    W  = RF3*PI/(RF4+RF4) ! angular speed omega

    !! Calculation of trajectory position

    if (TF > RF2) then

       FN = RF4*(RF2 - PI/W)

    else if (TF > RF2 - PI/W) then

       FN = RF4*(RF2 - PI/W) - 0.25_dp*RF3*trigFunc(RF2 - TF)

    else if (TF > PI/W) then

       FN = RF4*(TF - PI/(W+W))

    else if (TF > 0.0_dp) then

       FN = 0.25_dp*RF3*trigFunc(TF)

    end if

    return

915 call ffamsg_list ('          FUNC22, baseId =',ikf)

  contains

    !> @brief Evaluates the trigonometric part of the trajectory function.
    function trigFunc(x) result(f)
      real(dp), intent(in) :: x
      real(dp)             :: f
      f = x*x - (1.0_dp - cos((W+W)*x))/((W+W)*W)
    end function trigFunc

  end function FUNC22

end module ExplicitFunctionsModule
