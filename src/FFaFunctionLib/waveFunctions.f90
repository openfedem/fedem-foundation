!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file waveFunctions.f90
!> @brief Global functions callable from C++ code.
!>
!> @details This file contains global function wrappers of subroutines from the
!> wavefunctionsmodule, such that they can be invoked from C++ or python.

!!==============================================================================
!> @brief Initializes parameters for a wave function from given file.
!> @param[in] fName Name of file to read parameters from
!> @param[in] nWave Number of wave components to read
!> @param[in] rSeed Seed for (pseudo-)random phase shift
!> @param[out] rfunc Wave function parameters (amplitude, frequency, ...)
!> @param[out] ierr Error flag

subroutine initWaveFuncFromFile (fName, nWave, rSeed, rfunc, ierr)

  use WaveFunctionsModule, only : dp, initFUNC4

  character(len=*), intent(in)  :: fName
  integer         , intent(in)  :: nWave, rSeed
  real(dp)        , intent(out) :: rfunc(3,nWave)
  integer         , intent(out) :: ierr

  !! --- Logic section ---

  call initFUNC4 (fName,0.0_dp,0.0_dp,rfunc,ierr,rSeed)

end subroutine initWaveFuncFromFile


!!==============================================================================
!> @brief Initializes parameters for a wave function from a wave spectrum.
!> @param[in] iopW Wave spectrum model
!> @param[in] nWave Number of wave components to generate
!> @param[in] nDir Number of wave directions (for @a iopW > 4 only)
!> @param[in] sprExp Wave spreading exponent (for @a iopW > 4 and @a nDir > 1)
!> @param[in] rSeed Seed for (pseudo-)random phase shift
!> @param[out] rfunc Wave function parameters (amplitude, frequency, ...)
!> @param[out] ierr Error flag

subroutine initWaveFuncSpectrum (iopW, nWave, nDir, sprExp, rSeed, rfunc, ierr)

  use WaveFunctionsModule, only : dp, twoPi, initFUNC4

  integer , intent(in)    :: iopW, nWave, nDir, sprExp, rSeed
  real(dp), intent(inout) :: rfunc(3,nWave*nDir)
  integer , intent(out)   :: ierr

  !! Local variables
  real(dp) :: H3, T1, w0, w1, dw, Ga

  !! --- Logic section ---

  H3 = rfunc(1,1)
  T1 = rfunc(2,1)
  Ga = rfunc(2,2)
  if (rfunc(3,1) > 0.0_dp .and. rfunc(1,2) > 0.0_dp) then
     w0 = twoPi/rfunc(3,1)
     w1 = twoPi/rfunc(1,2)
  else
     !! Assuming the frequency range is specified directly
     w0 = abs(rfunc(3,1))
     w1 = abs(rfunc(1,2))
  end if
  dw = (w1-w0)/real(nWave,dp)
  if (iopW > 4) then
     !! Use new implementation, based on the DNV RP 205
     if (iopW == 5 .or. iopW == 6) Ga = 1.0_dp ! Pierson-Moskowitz
     if (iopW == 5 .or. iopW == 7) w0 = -w0 ! Use constant frequency intervals
     call initFUNC4 (H3,T1,Ga,w0,dw,0.0_dp,0.0_dp,rfunc,ierr,rSeed,nDir,sprExp)
  else
     !! Old implementation, based on O. Faltinsen's book
     call initFUNC4 (iopW,H3,T1,Ga,w0,dw,0.0_dp,0.0_dp,rfunc,ierr,rSeed)
  end if

end subroutine initWaveFuncSpectrum


!!==============================================================================
!> @brief Initializes parameters for a non-linear wave function.
!> @param[in] iopW Wave model option
!> @param[in] g Gravity constant
!> @param[in] D Water depth
!> @param RFUNC Wave function parameters
!> @param[out] stat Error flag

subroutine initNonlinWaveFunc (iopW, g, D, RFUNC, stat)

  use WaveFunctionsModule, only : dp, initFUNC7, initFUNC8

  integer , intent(in)    :: iopW
  real(dp), intent(in)    :: g, D
  real(dp), intent(inout) :: rfunc(*)
  integer , intent(out)   :: stat

  !! --- Logic section ---

  if (iopW == 7) then
     call initFUNC7 (rfunc(2),rfunc(1),rfunc(3),g,D,RFUNC,stat)
  else
     call initFUNC8 (rfunc(2),rfunc(1),rfunc(3),g,D,RFUNC,stat)
  end if
  if (stat > 0) stat = 2*stat+1 ! = size(rfunc)

end subroutine initNonlinWaveFunc


!!==============================================================================
!> @brief Initializes parameters for an embedded non-linear wave function.
!> @param[in] iopW Wave model option
!> @param[in] nWave Number of wave components
!> @param[in] rSeed Seed for (pseudo-)random phase shift
!> @param[in] g Gravity constant
!> @param[in] D Water depth
!> @param ifunc Integer wave function parameters
!> @param rfunc Real wave function parameters
!> @param[out] ierr Error flag

subroutine initEmbeddedWave (iopW, nWave, rSeed, g, D, ifunc, rfunc, ierr)

  use WaveFunctionsModule, only : dp, twoPi, initFUNC9
  use FFaMsgInterface    , only : ffamsg_list

  integer , intent(in)    :: iopW, nWave, rSeed
  real(dp), intent(in)    :: g, D
  integer , intent(inout) :: ifunc(*)
  real(dp), intent(inout) :: rfunc(*)
  integer , intent(out)   :: ierr

  !! Local variables
  integer  :: nS, nR
  real(dp) :: H3, T1, w0, w1, dw, Ga, ramp(2)
  real(dp), allocatable :: sfunc(:,:)

  !! --- Logic section ---

  nS = ifunc(4)
  allocate(sfunc(3,nS),stat=ierr)
  if (ierr /= 0) then
     call ffamsg_list ('initEmbeddedWave: Allocation failure',ierr)
     ierr = -abs(ierr)
     return
  else
     sfunc = reshape(rfunc(8:7+3*nS),(/3,nS/))
  end if

  nR = ifunc(5)
  H3 = rfunc(1)
  T1 = rfunc(2)
  Ga = rfunc(5)
  if (rfunc(3) > 0.0_dp .and. rfunc(4) > 0.0_dp) then
     w0 = twoPi/rfunc(3)
     w1 = twoPi/rfunc(4)
  else
     !! Assuming the frequency range is specified directly
     w0 = abs(rfunc(3))
     w1 = abs(rfunc(4))
  end if
  dw = (w1-w0)/real(nWave,dp)
  ramp = rfunc(6:7)
  if (iopW == 5 .or. iopW == 6) Ga = 1.0_dp ! Pierson-Moskowitz
  if (iopW == 5 .or. iopW == 7) w0 = -w0 ! Use constant frequency intervals
  call initFUNC9 (iopW,3,nWave,H3,T1,Ga,w0,dw,g,D,ramp,sfunc, &
       &          ifunc(1:4+nS),rfunc(1:nR),ierr,rSeed)
  deallocate(sfunc)

end subroutine initEmbeddedWave


!!==============================================================================
!> @brief Evaluates a wave profile function.
!> @param[in] iopW Wave model option
!> @param[in] ldi Number of integer parameters
!> @param[in] ldr Number of real parameters per wave component
!> @param[in] nWave Number of wave components
!> @param[in] nDir Number of wave directions
!> @param[in] ifunc Integer wave function parameters
!> @param[in] rfunc Real wave function parameters
!> @param[in] g Gravity constant
!> @param[in] d Water depth
!> @param[in] x Position to evaluate wave profile at
!> @param[in] t Time to evaluate wave profile at
!> @return Wave elevation

function waveProfile (iopW, ldi, ldr, nWave, nDir, ifunc, rfunc, &
     &                g, d, x, t) result(h)

  use WaveFunctionsModule    , only : dp, deepWave, deepWaves
  use WaveFunctionsModule    , only : finiteDepthWave, finiteDepthWaves
  use WaveFunctionsModule    , only : stokes2Wave, stokes5Wave, streamWave
  use WaveFunctionsModule    , only : embeddedWave, userDefinedWave
  use ExplicitFunctionsModule, only : WAVE_STOKES5_p, WAVE_STREAMLINE_p
  use ExplicitFunctionsModule, only : WAVE_EMBEDDED_p, USER_DEFINED_p

  integer , intent(in) :: iopW, ldi, ldr, nWave, ifunc(ldi)
  real(dp), intent(in) :: rfunc(ldr,nWave,nDir), g, d, x(3), t
  integer              :: ierr
  real(dp)             :: h, v(3), a(3), Xt(4)
  real(dp), parameter  :: z = 100000.0_dp
  logical , parameter  :: atS_p = .false., noWS_p = .false.

  !! --- Logic section ---

  select case (iopW)
  case (0)
     if (nDir < 2) then
        call deepWave (rfunc(:,:,1),g,x(1),z,t,atS_p,noWS_p, &
             &         h,v(1),v(3),a(1),a(3))
     else
        call deepWaves (rfunc,g,x(1),x(2),z,t,atS_p,noWS_p, &
             &          h,v(1),v(2),v(3),a(1),a(2),a(3))
     end if
  case (1)
     if (nDir < 2) then
        call finiteDepthWave (rfunc(:,:,1),g,d,x(1),z,t,atS_p,noWS_p, &
             &                h,v(1),v(3),a(1),a(3))
     else
        call finiteDepthWaves (rfunc,g,d,x(1),x(2),z,t,atS_p,noWS_p, &
             &                 h,v(1),v(2),v(3),a(1),a(2),a(3))
     end if
  case (2)
     call stokes2Wave (rfunc(:,:,1),g,d,x(1),z,t,atS_p,noWS_p, &
          &            h,v(1),v(3),a(1),a(3))
  case (WAVE_STOKES5_p)
     call stokes5Wave (rfunc(:,1,1),d,x(1),z,t,atS_p, &
          &            h,v(1),v(3),a(1),a(3))
  case (WAVE_STREAMLINE_p)
     call streamWave (rfunc(:,1,1),d,x(1),z,t,atS_p, &
          &           h,v(1),v(3),a(1),a(3))
  case (WAVE_EMBEDDED_p)
     call embeddedWave (ifunc,rfunc(:,1,1),g,d,x(1),z,t,atS_p,noWS_p, &
          &             h,v(1),v(3),a(1),a(3))
  case (USER_DEFINED_p)
     Xt(1:3) = x; Xt(4) = t
     call userDefinedWave (1,ifunc,rfunc(:,1,1),g,d,Xt,atS_p,h,v,a,ierr)
  case default
     h = 0.0_dp ! Invalid wave function type
  end select

end function waveProfile


!!==============================================================================
!> @brief Evaluates a wave profile.
!> @param[in] iopW Wave model option
!> @param[in] ldi Number of integer parameters
!> @param[in] ldr Number of real parameters per wave component
!> @param[in] nWave Number of wave components
!> @param[in] nDir Number of wave directions
!> @param[in] ifunc Integer wave function parameters
!> @param[in] rfunc Real wave function parameters
!> @param[in] g Gravity constant
!> @param[in] d Water depth
!> @param[in] x Position to evaluate wave profile at
!> @param[in] t Time to evaluate wave profile at
!> @param[out] h Wave elevation
!> @param[out] v Water particle velocity
!> @param[out] a Water particle acceleration

subroutine evalWave (iopW, ldi, ldr, nWave, nDir, ifunc, rfunc, &
     &               g, d, x, t, h, v, a)

  use WaveFunctionsModule    , only : dp, deepWave, deepWaves
  use WaveFunctionsModule    , only : finiteDepthWave, finiteDepthWaves
  use WaveFunctionsModule    , only : stokes2Wave, stokes5Wave, streamWave
  use WaveFunctionsModule    , only : embeddedWave, userDefinedWave
  use ExplicitFunctionsModule, only : WAVE_STOKES5_p, WAVE_STREAMLINE_p
  use ExplicitFunctionsModule, only : WAVE_EMBEDDED_p, USER_DEFINED_p

  integer , intent(in)  :: iopW, ldi, ldr, nWave, ifunc(ldi)
  real(dp), intent(in)  :: rfunc(ldr,nWave,nDir), g, d, x(3), t
  integer               :: ierr
  real(dp)              :: Xt(4)
  real(dp), intent(out) :: h, v(3), a(3)
  logical , parameter   :: atS_p = .false., noWS_p = .false.

  !! --- Logic section ---

  v(2) = 0.0_dp
  a(2) = 0.0_dp
  select case (iopW)
  case (0)
     if (nDir < 2) then
        call deepWave (rfunc(:,:,1),g,x(1),x(3),t,atS_p,noWS_p, &
             &         h,v(1),v(3),a(1),a(3))
     else
        call deepWaves (rfunc,g,x(1),x(2),x(3),t,atS_p,noWS_p,h, &
             &          v(1),v(2),v(3),a(1),a(2),a(3))
     end if
  case (1)
     if (nDir < 2) then
        call finiteDepthWave (rfunc(:,:,1),g,d,x(1),x(3),t,atS_p,noWS_p, &
             &                h,v(1),v(3),a(1),a(3))
     else
        call finiteDepthWaves (rfunc,g,d,x(1),x(2),x(3),t,atS_p,noWS_p, &
             &                 h,v(1),v(2),v(3),a(1),a(2),a(3))
     end if
  case (2)
     call stokes2Wave (rfunc(:,:,1),g,d,x(1),x(3),t,atS_p,noWS_p, &
          &            h,v(1),v(3),a(1),a(3))
  case (WAVE_STOKES5_p)
     call stokes5Wave (rfunc(:,1,1),d,x(1),x(3),t,atS_p,h,v(1),v(3),a(1),a(3))
  case (WAVE_STREAMLINE_p)
     call streamWave (rfunc(:,1,1),d,x(1),x(3),t,atS_p,h,v(1),v(3),a(1),a(3))
  case (WAVE_EMBEDDED_p)
     call embeddedWave (ifunc,rfunc(:,1,1),g,d,x(1),x(3),t,atS_p,noWS_p, &
          &             h,v(1),v(3),a(1),a(3))
  case (USER_DEFINED_p)
     Xt(1:3) = x; Xt(4) = t
     call userDefinedWave (1,ifunc,rfunc(:,1,1),g,d,Xt,atS_p,h,v,a,ierr)
  case default
     h = 0.0_dp ! Invalid wave function type
  end select

end subroutine evalWave
