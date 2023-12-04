!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

program funcTest

  use explicitfunctionsmodule, only : funcValue, funcIntegral, funcDerivative
  use wavefunctionsmodule    , only : initFUNC4, deepWave, dp

  integer  :: ierr, ifunc(5), N, IOP
  real(dp) :: x, dx, f(0:3), H3, T1, Omega0, dOmega, gamma
  real(dp), pointer :: rfunc(:), wfunc(:,:)
  character(len=16) :: argv
  character(len=64) :: fName

  N = 0
  x = 0.05_dp
  dx = 0.0001_dp
  ifunc(1) = 10
  ifunc(2:5) = 1
  ifunc(3) = 23
  ifunc(4) = 4
  if (iargc() > 5) then
     call getarg(1,argv); read(argv,*) IOP
     call getarg(2,argv); read(argv,*) H3
     call getarg(3,argv); read(argv,*) T1
     call getarg(4,argv); read(argv,*) Omega0
     call getarg(5,argv); read(argv,*) dOmega
     call getarg(6,argv); read(argv,*) N
     allocate(wfunc(3,N))
     if (IOP == 5 .or. IOP == 6) then
        gamma = 1.0_dp
     else if (iargc() < 7) then
        gamma = 3.3_dp
     else
        call getarg(7,argv); read(argv,*) gamma
     end if
     if (IOP == 5 .or. IOP == 7) Omega0 = -Omega0
     if (IOP > 4) then
        call initFUNC4(H3,T1,Gamma,Omega0,dOmega,9.81_dp,100.0_dp,wfunc,ierr)
     else
        call initFUNC4(IOP,H3,T1,3.3_dp,Omega0,dOmega,9.81_dp,100.0_dp,wfunc,ierr)
     end if
     if (ierr /= 0) stop 'initFUNC4'
     rfunc => matToVec(wfunc,3,N)
  else if (iargc() > 1) then
     call getarg(1,fName)
     call getarg(2,argv); read(argv,*) N
     allocate(wfunc(3,N))
     call initFUNC4(fName,9.81_dp,100.0_dp,wFunc,ierr)
     if (ierr /= 0) stop 'initFUNC4'
     rfunc => matToVec(wfunc,3,N)
  else if (iargc() == 1) then
     x = 0.0_dp
     call getarg(1,argv); read(argv,*) dx
     allocate(rfunc(8))
     ifunc(2) = 2
     rfunc = 0.0_dp
     rfunc(3) = 1.0_dp
     rfunc(4) = 0.01_dp
     rfunc(5) = 1.0_dp
     rfunc(6) = 0.02_dp
     rfunc(7) = 1.5_dp
     rfunc(8) = 0.03_dp
     print*,'rfunc:',rfunc
     print*
  else
     allocate(rfunc(8))
     rfunc = 0.0_dp
     rfunc(1) = 0.0507_dp
     rfunc(3) = 0.0537_dp
     rfunc(4) = 1000000.0_dp
     print*,'rfunc:',rfunc
     print*
  end if

  do i = 1,110
  call funcValue(90,ifunc,rfunc,x,f(0),ierr)
  if (N == 0) then
  call funcIntegral(90,1,ifunc,rfunc,x,f(1),ierr)
  call funcIntegral(90,2,ifunc,rfunc,x,f(2),ierr)
  call funcDerivative(90,1,ifunc,rfunc,x,f(3),ierr)
  print 2,x,f
  else
  print 2,x,f(0)
  endif
  x = x + dx
  end do
2 format(f6.3,1p4e18.11)

  if (N > 0) then
     print*,'Testing deepWave performance'
     x = 0.0_dp
     do i = 1,1000
     call deepWave(wfunc,9.81_dp,0.0_dp,-5.0_dp,x,.true.,.false., &
          &        h3,t1,f(0),f(1),f(2),gamma)
     x = x + 0.01_dp
     if (mod(i,10) == 0) print 1,i,x,h3,t1,f,gamma
1    format(i6,f6.2,1P6E11.3)
     end do
     print*,'Done'
  end if

contains

  function matToVec (mat,m,n)
    integer , intent(in) :: m, n
    real(dp), intent(in), target :: mat(m*n)
    real(dp), pointer :: matToVec(:)
    matToVec => mat
  end function matToVec

end program funcTest
