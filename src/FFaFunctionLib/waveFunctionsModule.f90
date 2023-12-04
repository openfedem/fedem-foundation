!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

module WaveFunctionsModule

  !!============================================================================
  !! This module contains subroutines for initialization of real parameters for
  !! the explicit wave function (FUNC4). It also has subroutines for evaluating
  !! the wave function at a given point in time and space. The FUNC4 interface
  !! of the ExplicitFunctionsModule only yields the free surface level at a
  !! given time and x = 0, and infinite water depth, whereas the routines herein
  !! can be used for any x and finite depth. Moreover, subroutines returning the
  !! corresponding velocity and acceleration for any depth z <= 0 are provided.
  !!============================================================================

  implicit none

  private

  integer , parameter :: dp = kind(1.0D0) ! 8-byte real (double)
  real(dp), parameter :: pi = 3.141592653589793238_dp
  real(dp), parameter :: twoPi = pi+pi

  interface initFUNC4
     module procedure initFUNC4fromFile
     module procedure initSpectrumDNV
     module procedure initSpectrumOF
  end interface

  public :: dp, twoPi, initFUNC4, initFUNC7, initFUNC8, initFUNC9
  public :: deepWave, deepWaves, finiteDepthWave, finiteDepthWaves
  public :: stokes2Wave, stokes5Wave, streamWave, embeddedWave, userDefinedWave
  public :: waveNumber, checkDepth


contains

  subroutine initFUNC4fromFile (FNAME,G,D,RFUNC,IERR,RSEED)

    !!==========================================================================
    !! Initializes function parameters for a wave function from the given file.
    !! The file may consist of either two, three or four columns of data.
    !! The number of columns may be specified on the first line as #ncol <ncol>.
    !! If the first line does not contain the keyword #ncol, a two-column file
    !! is assumed. The phase shift is then generated randomly. The wave number
    !! is calculated internally unless specified in the fourth column.
    !!   col #1 : Amplitude   [m]
    !!   col #2 : Frequency   [Hz]
    !!   col #3 : Phase shift [0,1]
    !!   col #4 : Wave number [1/m]
    !!
    !! Programmer : Knut Morten Okstad          date/rev : 16 January 2007 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    character(len=*) , intent(in)  :: FNAME
    real(dp)         , intent(in)  :: G, D
    real(dp)         , intent(out) :: RFUNC(:,:)
    integer          , intent(out) :: IERR
    integer, optional, intent(in)  :: RSEED

    !! Local variables
    integer          :: iunit, i, lf, ncol, Nseed, seed(16)
    logical          :: lopen
    real(dp)         :: Hs
    character(len=8) :: chnum

    !! --- Logic section ---

    ncol = size(RFUNC,1)
    if (ncol < 3) then
       ierr = -1
       call ffamsg_list ('Error :   Invalid dimension on RFUNC')
       call ffamsg_list ('          initFUNC4, size(RFUNC,1) =',ncol)
    end if

    RFUNC = 0.0_dp
    iunit = 10
    lopen = .true.
    do while (lopen)
       iunit = iunit + 1
       inquire(iunit,OPENED=lopen)
    end do

    lf = len_trim(FNAME) ! Bugfix #481: trim(FNAME) crashes on windows
    call ffamsg_list ('  -> Reading wave spectrum file: '//FNAME(1:lf))
    open(iunit,FILE=FNAME(1:lf),STATUS='OLD',ERR=900,IOSTAT=IERR)
    read(iunit,'(a)',ERR=910,IOSTAT=IERR) chnum
    if (chnum(1:5) == '#ncol' .or. chnum(1:5) == '#NCOL') then
       read(chnum(6:),*,ERR=910) i
       if (ncol > i) ncol = i
    else
       ncol = 2 ! Two-column file (default)
       rewind(iunit)
    end if

    select case (ncol)
    case (2); call ffamsg_list ('     Two-column input (A,f)')
    case (3); call ffamsg_list ('     Three-column input (A,f,eps)')
    case (4); call ffamsg_list ('     Four-column input (A,f,eps,kappa)')
    end select
    call ffamsg_list ('     Number of wave components: ',size(RFUNC,2))

    do i = 1, size(RFUNC,2)
       read(iunit,*,ERR=910,IOSTAT=IERR) RFUNC(1:ncol,i)
    end do
    close(iunit)

    !! Calculate the significant wage height
    Hs = 4.0_dp*sqrt(0.5_dp*sum(RFUNC(1,:)*RFUNC(1,:)))
    write(chnum,"(F8.2)") Hs
    call ffamsg_list ('     Significant wave height, Hs='//adjustl(chnum))

    if (ncol == 2) then
       if (present(RSEED)) then
          seed = RSEED
       else
          seed = 0
       end if
       !! Generate pseudo-random phase shift
       call random_seed (SIZE=Nseed)
       call random_seed (PUT=seed(1:Nseed))
       call random_number (RFUNC(3,:))
    end if

    RFUNC(2:3,:) = RFUNC(2:3,:) * twoPi

    IERR = 0
    if (size(RFUNC,1) > 3 .and. ncol < 4) then
       !! Calculate the wave number for each component
       do i = 1, size(RFUNC,2)
          RFUNC(4,i) = waveNumber(RFUNC(2,i),G,abs(D))
          if (checkDepth(D,RFUNC(4,i))) IERR = IERR + 1
       end do
    end if

    return

900 call ffamsg_list ('Error :   Failed to open wave data file '//FNAME(1:lf))
    call ffamsg_list ('          initFUNC4, IERR =',IERR)
    IERR = -abs(IERR)
    return

910 call ffamsg_list ('Error :   EOF detected while reading '//FNAME(1:lf))
    call ffamsg_list ('          initFUNC4, IERR =',IERR)
    IERR = -abs(IERR)

  end subroutine initFUNC4fromFile


  subroutine initSpectrumOF (IOP,H3,T1,Gamma,Omega0,dOmega,g,D,RFUNC,IERR,RSEED)

    !!==========================================================================
    !! Initializes a wave function based on statistical parameters.
    !! Ref: O. Faltinsen, Sea Loads on Ships and Offshore Structures, pp. 23-26.
    !!
    !! IOP    - Wave spectrum model
    !!          = 1 : Pierson-Moskowitz with constant frequency intervals
    !!          = 2 : Pierson-Moskowitz with random frequency intervals
    !!          = 3 : JONSWAP spectrum with constant frequency intervals
    !!          = 4 : JONSWAP spectrum with random frequency intervals
    !! H3     - Significant wave height (mean of the one third highest waves)
    !! T1     - Mean wave period
    !! Gamma  - Spectral peakedness
    !! Omega0 - Lowest frequency in wave spectrum
    !! dOmega - Frequency increment between successive wave components
    !! g      - Gravity constant
    !! D      - Sea depth for finite depth waves
    !! N      - Number of wave components
    !!
    !! Programmer : Knut Morten Okstad          date/rev : 29 January 2007 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    integer ,           intent(in)  :: IOP
    real(dp),           intent(in)  :: H3, T1, Gamma, Omega0, dOmega, g, D
    real(dp),           intent(out) :: RFUNC(:,:)
    integer ,           intent(out) :: IERR
    integer , optional, intent(in)  :: RSEED

    !! Local variables
    integer            :: i, N, Nseed, seed(16)
    real(dp)           :: A1, A2, OmegaT4, Y
    character(len=128) :: chpar

    !! --- Logic section ---

    IERR = 0
    N    = size(RFUNC,2)
    write(chpar,600) H3,T1,Gamma,Omega0,dOmega,N
600 format(5X,'H_1/3 =',F6.2,'  T_1 =',F6.2,'  gamma =',F6.2, &
         '  omega_min =',F6.2,'  domega =',1PE10.3,'  N =',I6)
    call ffamsg_list ('  -> Computing wave spectrum, IOP =',IOP)
    call ffamsg_list (trim(chpar))

    if (N < 1)             IERR = IERR - 1
    if (T1 <= 0.0_dp)      IERR = IERR - 2
    if (dOmega <= 0.0_dp)  IERR = IERR - 8
    if (size(RFUNC,1) < 3) IERR = IERR - 16
    if (ierr < 0) then
       call ffamsg_list ('Error :   Invalid input parameters')
       call ffamsg_list ('          initFUNC4, IERR =',IERR)
       return
    end if

    if (present(RSEED)) then
       seed = RSEED
       write(chpar,"(5X,'Random seed =',I6)") RSEED
       call ffamsg_list (trim(chpar))
    else
       seed = 0
    end if
    call random_seed (SIZE=Nseed)
    call random_seed (PUT=seed(1:Nseed))

    if (IOP == 1 .or. IOP == 3) then
       !! Constant frequency intervals
       do i = 1, N
          RFUNC(2,i) = Omega0 + dOmega*real(i-1,dp)
       end do
    else
       !! Random frequency intervals
       call random_number (RFUNC(2,:))
       do i = 1, N
          RFUNC(2,i) = Omega0 + dOmega*(RFUNC(2,i)+real(i,dp)-1.5_dp)
       end do
    end if

    if (iop <= 2) then
       !! Pierson-Moskowitz
       A1 = 0.11_dp * twoPi ** 4.0_dp
       A2 = 0.44_dp * twoPi ** 4.0_dp
    else
       !! JONSWAP
       A1 = 155.0_dp
       A2 = 944.0_dp
    end if

    do i = 1, N
       if (RFUNC(2,i) <= 0.0_dp) then
          RFUNC(:,i) = 0.0_dp
          cycle
       end if
       OmegaT4 = (RFUNC(2,i)*T1) ** 4.0_dp
       RFUNC(1,i) = A1 * (H3*H3 / (RFUNC(2,i)*OmegaT4)) * exp(-A2/OmegaT4)
       if (IOP > 2) then
          if (RFUNC(2,i) <= 5.24_dp/T1) then
             Y = (0.191_dp*RFUNC(2,i)*T1-1.0_dp)/(sqrt(2.0_dp)*0.07_dp)
          else
             Y = (0.191_dp*RFUNC(2,i)*T1-1.0_dp)/(sqrt(2.0_dp)*0.09_dp)
          end if
          RFUNC(1,i) = RFUNC(1,i) * Gamma ** exp(-Y*Y)
       end if
       RFUNC(1,i) = sqrt(2.0_dp*RFUNC(1,i)*dOmega)
       if (size(RFUNC,1) > 3) then
          RFUNC(4,i) = waveNumber(RFUNC(2,i),g,abs(D))
          if (checkDepth(D,RFUNC(4,i))) IERR = IERR + 1
       end if
    end do
    if (IERR > 0) IERR = IERR + 3

    call random_number (RFUNC(3,:))
    RFUNC(3,:) = RFUNC(3,:) * twoPi

#ifdef FT_DEBUG
    write(6,"('#  i      A_i       omega_i      epsilon_i      k_i')")
    do i = 1, N
       write(6,"(I4,1P4E13.5)") i, RFUNC(:,i)
    end do
    call flush(6)
#endif

  end subroutine initSpectrumOF


  subroutine initSpectrumDNV (Hs,Tp,Gamma,Omega0,dOmega,g,D,RFUNC,IERR, &
       &                      RSEED,NWdir,SprExp)

    !!==========================================================================
    !! Initializes a wave function based on statistical parameters.
    !!
    !! References:
    !!    DNV Recommended practice 205, October 2010.
    !!    S. K. Chakrabarti, Hydrodynamics of Offshore Structures, pp. 113-116.
    !!
    !! Hs     - Significant wave height (mean of the one third highest waves)
    !! Tp     - The wave period corresponding to the peak frequency
    !! Gamma  - Spectral peakedness
    !!          = 1.0: Pierson-Moskowitz
    !!          > 1.0: JONSWAP
    !! Omega0 - Lowest frequency in wave spectrum
    !!          <= 0.0: Use constant frequency intervals
    !! dOmega - Frequency increment between successive wave components
    !! g      - Gravity constant
    !! D      - Sea depth for finite depth waves
    !! N      - Number of wave components
    !! Ndir   - Number of wave directions
    !! SprExp - Wave spreading exponent (for Ndir > 1)
    !!
    !! Programmer : Paul Anton Letnes           date/rev : 03 January 2013 / 2.0
    !!              Knut Morten Okstad          date/rev : 20 July    2015 / 2.1
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    real(dp),           intent(in)  :: Hs, Tp, Gamma, Omega0, dOmega, g, D
    real(dp),           intent(out) :: RFUNC(:,:)
    integer ,           intent(out) :: IERR
    integer , optional, intent(in)  :: RSEED, NWdir, SprExp

    !! Local variables
    integer            :: i, j, k, N, Ndir, Nseed, loc(1), seed(16)
    real(dp)           :: omega_p, m0, theta, dtheta, Df, GamRat
    character(len=128) :: chpar

    !! --- Logic section ---

    IERR = 0
    N    = size(RFUNC,2)
    Ndir = 1
    if (present(NWdir)) N = N / NWdir
    write(chpar,600) Hs,Tp,Gamma,abs(Omega0),dOmega,N
600 format(5X,'H_s =',F6.2,'  T_p =',F6.2,'  gamma =',F6.2, &
         '  omega_min =',F6.2,'  domega =',1PE10.3,'  N =',I6)
    call ffamsg_list ('  -> Computing JONSWAP wave spectrum')
    call ffamsg_list (trim(chpar))
    if (present(NWdir)) then
       Ndir = NWdir
       write(chpar,601) Ndir,SprExp
601    format(5X,'Ndir =',I3,'  SpreadExp =',I2)
       call ffamsg_list (trim(chpar))
    end if

    if (N < 1)             IERR = IERR - 1
    if (Tp <= 0.0_dp)      IERR = IERR - 2
    if (dOmega <= 0.0_dp)  IERR = IERR - 8
    if (size(RFUNC,1) < 3) IERR = IERR - 16
    if (Ndir < 1 .or. mod(Ndir,2) == 0) IERR = IERR - 32
    if (Ndir > 1 .and. present(SprExp)) then
       if (sprExp < 2 .or. mod(SprExp,2) == 1) IERR = IERR - 64
    end if
    if (ierr < 0) then
       call ffamsg_list ('Error :   Invalid input parameters')
       call ffamsg_list ('          initFUNC4, IERR =',IERR)
       return
    end if

    if (present(RSEED)) then
       seed = RSEED
       write(chpar,"(5X,'Random seed =',I6)") RSEED
       call ffamsg_list (trim(chpar))
    else
       seed = 0
    end if
    call random_seed (SIZE=Nseed)
    call random_seed (PUT=seed(1:Nseed))
    if (Ndir > 1) then
       !! Workaround: First call seems to always return 0.0 or 1.0
       call random_number (dtheta)
    end if

    !! Calculate the peak angular frequency
    omega_p = twoPi / Tp

    if (Ndir > 1) then
       !! Evaluate the gamma function ratio of the directionality function:
       !! Gamma(1+k)/(sqrt(pi)*Gamma(0.5+k)) = (4^k * (k!)^2) / (pi * (2*n)!)
       !!                                    = (PI_i=1,k { 4*i/(k+i) })/pi
       !! See https://en.wikipedia.org/wiki/Gamma_function
       if (present(SprExp)) then
          k = SprExp/2
          GamRat = 1.0_dp / pi
          do i = 1, k
             GamRat = GamRat * real(4*i,dp)/real(k+i,dp)
          end do
       else ! SprExp = 2 (k = 1)
          GamRat = 2.0_dp / pi ! = Gamma(2)/(sqrt(pi)*Gamma(1.5))
       end if
       dtheta = pi / real(Ndir+1,dp)
       theta = dtheta - 0.5_dp*pi
    else
       Df = 1.0_dp
       GamRat = 0.0_dp
       theta = 0.0_dp
       dtheta = 1.0_dp
    end if

    !! Loop over wave directions
    j = 0
    do k = 1, Ndir
       if (Ndir > 1) then
          !! Evaluate the directionality function D(theta)
          if (present(SprExp)) then
             Df = GamRat * cos(theta)**real(SprExp,dp)
          else ! SprExp = 2
             Df = GamRat * cos(theta)*cos(theta)
          end if
          write(chpar,602) theta*180_dp/pi,Df
602       format(5X,'theta =',F8.2,':  D(theta) =',1PE12.5)
          call ffamsg_list (trim(chpar))
       end if

       if (Omega0 <= 0.0_dp) then
          !! Constant frequency intervals
          RFUNC(2,j+1) = -Omega0
          do i = j+2, j+N
             RFUNC(2,i) = RFUNC(2,i-1) + dOmega
          end do
       else
          !! Random frequency intervals
          call random_number (RFUNC(2,j+1:j+N))
          do i = j+1, j+N
             RFUNC(2,i) = Omega0 + dOmega*(RFUNC(2,i)+real(i-j,dp)-1.5_dp)
          end do
       end if

       !! Calculate the wave spectrum and wave numbers
       do i = j+1, j+N
          if (RFUNC(2,i) <= 0.0_dp) then
             RFUNC(:,i) = 0.0_dp
          else
             if (Gamma <= 1.0_dp) then
                RFUNC(1,i) = PiersonMoskowitz(Hs,RFUNC(2,i),omega_p)*Df
             else
                RFUNC(1,i) = JONSWAP(Hs,RFUNC(2,i),omega_p,Gamma)*Df
             end if
             if (size(RFUNC,1) > 3) then
                RFUNC(4,i) = waveNumber(RFUNC(2,i),g,abs(D))
                if (checkDepth(D,RFUNC(4,i))) IERR = IERR + 1
             end if
          end if
       end do
       if (IERR > 0) IERR = IERR + 3

       !! Compute amplitudes from spectrum
       RFUNC(1,j+1:j+N) = sqrt(RFUNC(1,j+1:j+N) * dOmega*dtheta * 2.0_dp)
       !! TODO determine a better tolerance criterion?
       !! Perform other checks, e.g., m1, m2, m3? Wave steepness? Mean period?
       !! (Note: m4 and higher diverges)
       m0 = 0.5_dp * sum(RFUNC(1,j+1:j+N)**2.0_dp)
       if (abs(m0-Hs*Hs/16.0_dp) > 0.05_dp*m0 .and. Ndir == 1) then
          IERR = IERR + 1
          call ffamsg_list ('Warning : Cutoff in spectrum is too coarse '// &
               &            'or number of wave components is too small,')
          write(chpar,"('Hs =',F6.2,' m0 =',1PE12.5)") Hs,m0
          call ffamsg_list ('          '//trim(chpar))
       end if
       loc = maxloc(RFUNC(1,j+1:j+N))
       if (abs(RFUNC(2,loc(1)+j) - omega_p) > dOmega) then
          IERR = IERR + 2
          write(chpar,*) abs(RFUNC(2,loc(1)) - omega_p), dOmega, omega_p
          call ffamsg_list ('Warning : Spectrum peak is far off '// &
               &            'the specified value,')
          write(chpar,"('max{Omega} = Omega(',i6,') =',1PE12.5)") &
               &      loc(1), RFUNC(2,loc(1))
          call ffamsg_list ('          '//trim(chpar))
          write(chpar,"('dOmega =',1PE12.5,' Omega_p = ',E12.5)") &
               &      dOmega, omega_p
       end if

       !! Set the random phase angle, no input required
       call random_number (RFUNC(3,j+1:j+N))
       RFUNC(3,j+1:j+N) = RFUNC(3,j+1:j+N) * twoPi

#ifdef FT_DEBUG
       if (Ndir > 1) then
          write(6,"(/'# Wave spectrum for theta =',F8.2)") theta*180_dp/pi
       end if
       write(6,"('#  i      A_i       omega_i      epsilon_i      k_i')")
       do i = 1, N
          write(6,"(I4,1P4E13.5)") i, RFUNC(:,j+i)
       end do
       call flush(6)
#endif
       theta = theta + dtheta
       seed = seed + 1
       j = j + N
    end do

  contains

    function PiersonMoskowitz (Hs,omega,omega_p) result(S)
      real(dp), intent(in) :: Hs, omega, omega_p
      real(dp)             :: S
      S = 0.3125_dp * Hs*Hs * omega_p**4.0_dp * omega**(-5.0_dp) &
           * exp(-1.25_dp * (omega/omega_p)**(-4.0_dp))
    end function PiersonMoskowitz

    function JONSWAP (Hs,omega,omega_p,Gamma) result(S)
      real(dp), intent(in) :: Hs, omega, omega_p, Gamma
      real(dp)             :: S, sigma, A_gamma, Gamma_power
      if (omega <= omega_p) then
         sigma = 0.07_dp
      else
         sigma = 0.09_dp
      end if
      A_gamma = 1.0_dp - 0.287_dp * log(Gamma)
      Gamma_power = exp(-0.5_dp * ((omega-omega_p) / (sigma*omega_p))**2.0_dp)
      S = A_gamma * PiersonMoskowitz(Hs,omega,omega_p) * Gamma**Gamma_power
    end function JONSWAP

  end subroutine initSpectrumDNV


  function waveNumber (omega,g,d) result(k)

    !!==========================================================================
    !! Calculates the wave number (k) for finite depth waves by Newton-Raphson.
    !! If the water depth (d) is zero, infinite depth is assumed.
    !!
    !! omega - Angular frequency
    !! g     - Gravity constant
    !! d     - Water depth
    !!
    !! Programmer : Arne Rekdal                     date/rev : 05 Oct 2010 / 1.0
    !!==========================================================================

    real(dp), intent(in) :: omega, g, d

    !! Local variables
    integer  :: i
    real(dp) :: k, f0, df0, tkd

    !! Newton-Raphson tolerance criterion
    integer , parameter :: maxIter_p = 100
    real(dp), parameter :: tol_p = 1.0e-6_dp

    !! --- Logic section ---

    k = omega*omega/g
    if (d < tol_p) return

    do i = 1, maxIter_p
       tkd = tanh(k*d)
       f0  = omega*omega - g*k*tkd
       df0 = g*(tkd + k*d*(1.0_dp - tkd*tkd))
       k   = k + f0/df0
       if (abs(f0) < tol_p) exit
    end do

  end function waveNumber


  function checkDepth (d,k)

    !!==========================================================================
    !! Checks that the finite water depth is not too large to cause overflow.
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 25 Jan 2013 / 1.0
    !!              Knut Morten Okstad              date/rev : 02 Dec 2013 / 1.1
    !!==========================================================================

    real(dp), intent(in) :: d, k

    !! Local variables
    logical             :: checkDepth
    real(dp), parameter :: shmax = asinh(huge(1.0_dp)) ! limit on sinh argument

    !! --- Logic section ---

    checkDepth = k*d >= shmax

  end function checkDepth


  subroutine deepWave (RFUNC,g,x,z,t,atSurface,noWheelerStretching, &
       &               h,u,w,du,dw,p)

    !!==========================================================================
    !! Evaluates the wave profile and the associated particle velocity
    !! and acceleration at a given spatial point {x,0,z} and time, t.
    !! Assuming deep water waves, i.e., 2*depth > largest wave length.
    !! z=0 represents the mean free surface level and z is positive upwards.
    !!
    !! Reference: O. M. Faltinsen,
    !! "Sea Loads on Ships and Offshore Structures", Table 2.1, page 16.
    !!
    !! RFUNC  - Wave data, output from initFUNC4
    !! g      - Gravity constant
    !! x, z   - Current position
    !! t      - Current time
    !! h      - Current wave height above z=0 (depends on x and t only)
    !! u, w   - Current particle velocity in local x and z directions
    !! du, dw - Current particle acceleration in local x and z directions
    !! p      - Current dynamic pressure (optional)
    !!
    !! Programmer : Knut Morten Okstad            date/rev : 25 March 2009 / 1.0
    !!==========================================================================

    real(dp), intent(in)  :: RFUNC(:,:), g, x, z, t
    logical , intent(in)  :: atSurface, noWheelerStretching
    real(dp), intent(out) :: h, u, w, du, dw
    real(dp), optional, intent(out) :: p

    !! Local variables
    integer  :: i, ncmp
    real(dp) :: omega, eps, k, B, c, s, zz

    !! --- Logic section ---

    h  = 0.0_dp
    u  = 0.0_dp
    w  = 0.0_dp
    du = 0.0_dp
    dw = 0.0_dp
    if (present(p)) p = 0.0_dp
    ncmp = size(RFUNC,2)

    !$OMP PARALLEL DO PRIVATE(i,omega),REDUCTION(+:h)
    do i = 1, ncmp
       omega = RFUNC(2,i)
       h = h + RFUNC(1,i)*sin(omega*(t-x*omega/g)+RFUNC(3,i))
    end do
    !$OMP END PARALLEL DO

    !! Calculate the effective water depth
    if (atSurface) then ! z = h
       if (noWheelerStretching) then
          zz = min(h,0.0_dp)
       else
          zz = 0.0_dp
       end if
    else
       if (h < z) then
          return ! we are above the water surface
       else if (noWheelerStretching) then
          zz = min(z,0.0_dp)
       else
          zz = z-h
       end if
    end if

    !$OMP PARALLEL DO PRIVATE(i,omega,eps,k,B,c,s),REDUCTION(+:u,w,du,dw)
    do i = 1, ncmp
       omega = RFUNC(2,i)
       eps = RFUNC(3,i)
       k  = omega*omega/g
       B  = omega*RFUNC(1,i)*exp(k*zz)
       c  = cos(omega*t+eps-k*x)
       s  = sin(omega*t+eps-k*x)
       u  = u  + B*s
       w  = w  + B*c
       du = du + omega*B*c
       dw = dw - omega*B*s
    end do
    !$OMP END PARALLEL DO

    if (present(p)) then
       !$OMP PARALLEL DO PRIVATE(i,omega,eps,k,B),REDUCTION(+:p)
       do i = 1, ncmp
          omega = RFUNC(2,i)
          eps = RFUNC(3,i)
          k = omega*omega/g
          B = omega*RFUNC(1,i)*exp(k*zz)
          p = p + B*sin(omega*t+eps-k*x)*g/omega
       end do
       !$OMP END PARALLEL DO
    end if

  end subroutine deepWave


  subroutine deepWaves (RFUNC,g,x,y,z,t,atSurface,noWheelerStretching, &
       &                h,u,v,w,du,dv,dw,p)

    !!==========================================================================
    !! Evaluates the wave profile and the associated particle velocity
    !! and acceleration at a given spatial point {x,y,z} and time, t.
    !! Assuming deep water waves, i.e., 2*depth > largest wave length.
    !! z=0 represents the mean free surface level and z is positive upwards.
    !! This subroutine also accounts for wave spreading through an outer loop.
    !!
    !! Reference: O. M. Faltinsen,
    !! "Sea Loads on Ships and Offshore Structures", Table 2.1, page 16.
    !!
    !! RFUNC      - Wave data, output from initFUNC4
    !! g          - Gravity constant
    !! x, y, z    - Current position
    !! t          - Current time
    !! h          - Current wave height above z=0 (depends on x, t and t only)
    !! u, v, w    - Current particle velocity in local directions
    !! du, dv, dw - Current particle acceleration in local directions
    !! p          - Current dynamic pressure (optional)
    !!
    !! Programmer : Knut Morten Okstad             date/rev : 15 July 2015 / 1.0
    !!==========================================================================

    real(dp), intent(in)  :: RFUNC(:,:,:), g, x, y, z, t
    logical , intent(in)  :: atSurface, noWheelerStretching
    real(dp), intent(out) :: h, u, v, w, du, dv, dw
    real(dp), optional, intent(out) :: p

    !! Local variables
    integer  :: i, j, ncmp, ndir
    real(dp) :: omega, phase, theta, dtheta, k, B, c, s, zz, r, dr

    !! --- Logic section ---

    h  = 0.0_dp
    u  = 0.0_dp
    v  = 0.0_dp
    w  = 0.0_dp
    du = 0.0_dp
    dv = 0.0_dp
    dw = 0.0_dp
    if (present(p)) p = 0.0_dp

    ncmp = size(RFUNC,2)
    ndir = size(RFUNC,3)
    dtheta = pi / real(ndir+1,dp)

    theta = dtheta - 0.5_dp*pi
    do j = 1, ndir
       !$OMP PARALLEL DO PRIVATE(i,omega,k,phase),REDUCTION(+:h)
       do i = 1, ncmp
          omega = RFUNC(2,i,j)
          k     = omega*omega/g
          phase = RFUNC(3,i,j) - k*(x*cos(theta) + y*sin(theta))
          h = h + RFUNC(1,i,j)*sin(omega*t + phase)
       end do
       !$OMP END PARALLEL DO
       theta = theta + dtheta
    end do

    !! Calculate the effective water depth
    if (atSurface) then ! z = h
       if (noWheelerStretching) then
          zz = min(h,0.0_dp)
       else
          zz = 0.0_dp
       end if
    else
       if (h < z) then
          return ! we are above the water surface
       else if (noWheelerStretching) then
          zz = min(z,0.0_dp)
       else
          zz = z-h
       end if
    end if

    theta = dtheta - 0.5_dp*pi
    do j = 1, ndir
       r = 0.0_dp
       dr = 0.0_dp
       !$OMP PARALLEL DO PRIVATE(i,omega,phase,k,B,c,s), &
       !$OMP REDUCTION(+:r,w,dr,dw)
       do i = 1, ncmp
          omega = RFUNC(2,i,j)
          k     = omega*omega/g
          phase = RFUNC(3,i,j) - k*(x*cos(theta) + y*sin(theta))
          k  = omega*omega/g
          B  = omega*RFUNC(1,i,j)*exp(k*zz)
          c  = cos(omega*t + phase)
          s  = sin(omega*t + phase)
          r  = r  + B*s
          w  = w  + B*c
          dr = dr + omega*B*c
          dw = dw - omega*B*s
       end do
       !$OMP END PARALLEL DO
       u  = u  +  r*cos(theta)
       v  = v  +  r*sin(theta)
       du = du + dr*cos(theta)
       dv = dv + dr*sin(theta)

       if (present(p)) then
          !$OMP PARALLEL DO PRIVATE(i,omega,k,phase,B),REDUCTION(+:p)
          do i = 1, ncmp
             omega = RFUNC(2,i,j)
             k     = omega*omega/g
             phase = RFUNC(3,i,j) - k*(x*cos(theta) + y*sin(theta))
             B = omega*RFUNC(1,i,j)*exp(k*zz)
             p = p + B*sin(omega*t + phase)*g/omega
          end do
          !$OMP END PARALLEL DO
       end if

       theta = theta + dtheta
    end do

  end subroutine deepWaves


  subroutine finiteDepthWave (RFUNC,g,d,x,z,t,atSurface,noWheelerStretching, &
       &                      h,u,w,du,dw,p)

    !!==========================================================================
    !! Evaluates the wave profile and the associated particle velocity
    !! and acceleration at a given spatial point {x,0,z} and time, t.
    !! Assuming finite depth water waves, linear wave theory.
    !!
    !! Reference: O. M. Faltinsen,
    !! "Sea Loads on Ships and Offshore Structures", Table 2.1, page 16.
    !!
    !! RFUNC  - Wave data, output from initFUNC4
    !! g      - Gravity constant
    !! d      - Water depth
    !! x, z   - Current position (z=0 mean free surface level, positive upwards)
    !! t      - Current time
    !! h      - Current wave height above z=0 (depends on x and t only)
    !! u, w   - Current particle velocity in local x and z directions
    !! du, dw - Current particle acceleration in local x and z directions
    !! p      - Current dynamic pressure (optional)
    !!
    !! Programmer : Arne Rekdal                     date/rev : 05 Oct 2010 / 1.0
    !!==========================================================================

    real(dp), intent(in)  :: RFUNC(:,:), g, d, x, z, t
    logical , intent(in)  :: atSurface, noWheelerStretching
    real(dp), intent(out) :: h, u, w, du, dw
    real(dp), optional, intent(out) :: p

    !! Local variables
    integer  :: i, ncmp
    logical  :: haveK
    real(dp) :: zz, omega, k, B, c, s, ch, sh

    !! --- Logic section ---

    h  = 0.0_dp
    u  = 0.0_dp
    w  = 0.0_dp
    du = 0.0_dp
    dw = 0.0_dp
    if (present(p)) p = 0.0_dp
    ncmp = size(RFUNC,2)
    haveK = size(RFUNC,1) > 3

    !$OMP PARALLEL DO PRIVATE(i,k),REDUCTION(+:h)
    do i = 1, ncmp
       if (haveK) then
          k = RFUNC(4,i)
       else
          k = waveNumber(RFUNC(2,i),g,d)
       end if
       h = h + RFUNC(1,i)*sin(RFUNC(2,i)*t+RFUNC(3,i)-k*x)
    end do
    !$OMP END PARALLEL DO

    !! Calculate the effective water depth
    if (atSurface) then ! z = h
       if (noWheelerStretching) then
          zz = min(d+h,d)
       else
          zz = d
       end if
    else
       if (z < -d .or. z > h) then
          return ! we are below the bottom or above the surface
       else if (noWheelerStretching) then
          zz = min(d+z,d)
       else
          zz = d + (z-h)*d/(d+h)
       end if
    end if

    !$OMP PARALLEL DO PRIVATE(i,omega,k,B,ch,sh,s,c),REDUCTION(+:u,w,du,dw)
    do i = 1, ncmp
       omega = RFUNC(2,i)
       if (haveK) then
          k = RFUNC(4,i)
       else
          k = waveNumber(omega,g,d)
       end if

       B  = omega*RFUNC(1,i)/sinh(k*d)
       ch = cosh(k*zz)
       sh = sinh(k*zz)
       s  = sin(omega*t+RFUNC(3,i)-k*x)
       c  = cos(omega*t+RFUNC(3,i)-k*x)

       u  = u  + B*ch*s
       w  = w  + B*sh*c
       du = du + omega*B*ch*c
       dw = dw - omega*B*sh*s
    end do
    !$OMP END PARALLEL DO

    if (present(p)) then
       !$OMP PARALLEL DO PRIVATE(i,omega,k,s),REDUCTION(+:p)
       do i = 1, ncmp
          omega = RFUNC(2,i)
          if (haveK) then
             k = RFUNC(4,i)
          else
             k = waveNumber(omega,g,d)
          end if
          s = sin(omega*t+RFUNC(3,i)-k*x)
          p = p + g*RFUNC(1,i)*cosh(k*zz)*s/cosh(k*d)
       end do
       !$OMP END PARALLEL DO
    end if

  end subroutine finiteDepthWave


  subroutine finiteDepthWaves (RFUNC,g,d,x,y,z,t, &
       &                       atSurface,noWheelerStretching, &
       &                       h,u,v,w,du,dv,dw,p)

    !!==========================================================================
    !! Evaluates the wave profile and the associated particle velocity
    !! and acceleration at a given spatial point {x,y,z} and time, t.
    !! Assuming finite depth water waves, linear wave theory.
    !! This subroutine also accounts for wave spreading through an outer loop.
    !!
    !! Reference: O. M. Faltinsen,
    !! "Sea Loads on Ships and Offshore Structures", Table 2.1, page 16.
    !!
    !! RFUNC      - Wave data, output from initFUNC4
    !! g          - Gravity constant
    !! d          - Water depth
    !! x, y, z    - Current position (z=0 mean free surface level)
    !! t          - Current time
    !! h          - Current wave height above z=0 (depends on x, y and t only)
    !! u, v, w    - Current particle velocity in local directions
    !! du, dv, dw - Current particle acceleration in local directions
    !! p          - Current dynamic pressure (optional)
    !!
    !! Programmer : Knut Morten Okstad             date/rev : 15 July 2015 / 1.0
    !!==========================================================================

    real(dp), intent(in)  :: RFUNC(:,:,:), g, d, x, y, z, t
    logical , intent(in)  :: atSurface, noWheelerStretching
    real(dp), intent(out) :: h, u, v, w, du, dv, dw
    real(dp), optional, intent(out) :: p

    !! Local variables
    integer  :: i, j, ncmp, ndir
    logical  :: haveK
    real(dp) :: omega, phase, theta, dtheta, zz, k, B, c, s, ch, sh, r, dr

    !! --- Logic section ---

    h  = 0.0_dp
    u  = 0.0_dp
    v  = 0.0_dp
    w  = 0.0_dp
    du = 0.0_dp
    dv = 0.0_dp
    dw = 0.0_dp
    if (present(p)) p = 0.0_dp
    ncmp = size(RFUNC,2)
    ndir = size(RFUNC,3)
    haveK = size(RFUNC,1) > 3
    dtheta = pi / real(ndir+1,dp)

    theta = dtheta - 0.5_dp*pi
    do j = 1, ndir
       !$OMP PARALLEL DO PRIVATE(i,omega,k,phase),REDUCTION(+:h)
       do i = 1, ncmp
          omega = RFUNC(2,i,j)
          if (haveK) then
             k = RFUNC(4,i,j)
          else
             k = waveNumber(omega,g,d)
          end if
          phase = RFUNC(3,i,j) - k*(x*cos(theta) + y*sin(theta))
          h = h + RFUNC(1,i,j)*sin(omega*t + phase)
       end do
       !$OMP END PARALLEL DO
       theta = theta + dtheta
    end do

    !! Calculate the effective water depth
    if (atSurface) then ! z = h
       if (noWheelerStretching) then
          zz = min(d+h,d)
       else
          zz = d
       end if
    else
       if (z < -d .or. z > h) then
          return ! we are below the bottom or above the surface
       else if (noWheelerStretching) then
          zz = min(d+z,d)
       else
          zz = d + (z-h)*d/(d+h)
       end if
    end if

    theta = dtheta - 0.5_dp*pi
    do j = 1, ndir
       r = 0.0_dp
       dr = 0.0_dp
       !$OMP PARALLEL DO PRIVATE(i,omega,k,phase,B,ch,sh,s,c), &
       !$OMP REDUCTION(+:r,w,dr,dw)
       do i = 1, ncmp
          omega = RFUNC(2,i,j)
          if (haveK) then
             k = RFUNC(4,i,j)
          else
             k = waveNumber(omega,g,d)
          end if
          phase = RFUNC(3,i,j) - k*(x*cos(theta) + y*sin(theta))

          B  = omega*RFUNC(1,i,j)/sinh(k*d)
          ch = cosh(k*zz)
          sh = sinh(k*zz)
          s  = sin(omega*t + phase)
          c  = cos(omega*t + phase)

          r  = r  + B*ch*s
          w  = w  + B*sh*c
          dr = dr + omega*B*ch*c
          dw = dw - omega*B*sh*s
       end do
       !$OMP END PARALLEL DO
       u  = u  +  r*cos(theta)
       v  = v  +  r*sin(theta)
       du = du + dr*cos(theta)
       dv = dv + dr*sin(theta)

       if (present(p)) then
          !$OMP PARALLEL DO PRIVATE(i,omega,k,phase),REDUCTION(+:p)
          do i = 1, ncmp
             omega = RFUNC(2,i,j)
             if (haveK) then
                k = RFUNC(4,i,j)
             else
                k = waveNumber(omega,g,d)
             end if
             phase = RFUNC(3,i,j) - k*(x*cos(theta) + y*sin(theta))
             p = p + RFUNC(1,i,j)*g*sin(omega*t + phase)*cosh(k*zz)/cosh(k*d)
          end do
          !$OMP END PARALLEL DO
       end if

       theta = theta + dtheta
    end do

  end subroutine finiteDepthWaves


  subroutine stokes2Wave (RFUNC,g,d,x,z,t,atSurface,noWheelerStretching, &
       &                  h,u,w,du,dw)

    !!==========================================================================
    !! Evaluates the wave profile and the associated particle velocity
    !! and acceleration at a given spatial point {x,0,z} and time, t.
    !! Assuming 2nd order Stokes wave theory.
    !!
    !! Reference: J. F. Wilson,
    !! "Dynamics of Offshore Structures", Table 3.2, page 71.
    !!
    !! RFUNC  - Wave data, output from initFUNC4
    !! g      - Gravity constant
    !! d      - Water depth
    !! x, z   - Current position (z=0 mean free surface level, positive upwards)
    !! t      - Current time
    !! h      - Current wave height above z=0 (depends on x and t only)
    !! u, w   - Current particle velocity in local x and z directions
    !! du, dw - Current particle acceleration in local x and z directions
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 04 Apr 2011 / 1.0
    !!==========================================================================

    real(dp), intent(in)  :: RFUNC(:,:), g, d, x, z, t
    logical , intent(in)  :: atSurface, noWheelerStretching
    real(dp), intent(out) :: h, u, w, du, dw

    !! Local variables
    integer  :: i, ncmp
    logical  :: haveK
    real(dp) :: zz, A, omega, alpha, k, kz
    real(dp) :: c, c2, s, s2, ch, ch2, sh, sh2, skd

    !! --- Logic section ---

    h  = 0.0_dp
    u  = 0.0_dp
    w  = 0.0_dp
    du = 0.0_dp
    dw = 0.0_dp
    ncmp = size(RFUNC,2)
    haveK = size(RFUNC,1) > 3

    !$OMP PARALLEL DO PRIVATE(i,A,omega,k,alpha,skd),REDUCTION(+:h)
    do i = 1, ncmp
       A     = RFUNC(1,i)
       omega = RFUNC(2,i)
       if (haveK) then
          k = RFUNC(4,i)
       else
          k = waveNumber(omega,g,d)
       end if
       alpha = k*x - omega*t + RFUNC(3,i)
       skd = sinh(k*d)
       h = h + A*(cos(alpha) + (A*k*cosh(k*d)/(16.0_dp*skd*skd*skd)) * &
            &                  (2.0_dp+cosh((k+k)*d)) * cos(alpha+alpha))
    end do
    !$OMP END PARALLEL DO

    !! Calculate the effective water depth
    if (atSurface) then ! z = h
       if (noWheelerStretching) then
          zz = min(d+h,d)
       else
          zz = d
       end if
    else
       if (z < -d .or. z > h) then
          return ! we are below the bottom or above the surface
       else if (noWheelerStretching) then
          zz = min(d+z,d)
       else
          zz = d + (z-h)*d/(d+h)
       end if
    end if

    !$OMP PARALLEL DO &
    !$OMP PRIVATE(i,A,omega,k,kz,alpha,s,c,s2,c2,skd,ch,sh,sh2,ch2), &
    !$OMP REDUCTION(+:u,w,du,dw)
    do i = 1, ncmp
       A     = RFUNC(1,i)
       omega = RFUNC(2,i)
       if (haveK) then
          k  = RFUNC(4,i)
       else
          k  = waveNumber(omega,g,d)
       end if
       kz    = k*zz
       alpha = k*x - omega*t + RFUNC(3,i)

       s = sin(alpha)
       c = cos(alpha)
       s2 = sin(alpha+alpha)
       c2 = cos(alpha+alpha)
       skd = sinh(k*d)
       ch  = cosh(kz)/skd
       sh  = sinh(kz)/skd
       sh2 = sinh(kz+kz)/(skd*skd*skd*skd)
       ch2 = cosh(kz+kz)/(skd*skd*skd*skd)

       u  = u  + omega*A*(ch*c + 0.75_dp*A*k*ch2*c2)
       w  = w  + omega*A*(sh*s + 0.75_dp*A*k*sh2*s2)
       du = du + omega*omega*A*(ch*s + 0.375_dp*A*k*ch2*s2)
       dw = dw - omega*omega*A*(sh*c + 0.375_dp*A*k*sh2*c2)
    end do
    !$OMP END PARALLEL DO

  end subroutine stokes2Wave


  subroutine initFUNC7 (H,T,eps,g,D,RFUNC,ierr)

    !!==========================================================================
    !! Computes coefficients for 5th order Stokes waves. The subroutine stokes5
    !! is from the code that was downloaded from the following web-page:
    !! http://www.civil.soton.ac.uk/hydraulics/download/downloadtable.htm
    !! Note: This web-page no longer exists (January 2015).
    !!
    !! H   - Wave height
    !! T   - Wave period
    !! eps - Phase shift
    !! g   - Gravity constant
    !! D   - Water depth
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 23 May 2011 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    real(dp), intent(in)  :: H, T, eps, g, D
    real(dp), intent(out) :: RFUNC(*)
    integer , intent(out) :: ierr

    !! Local variables
    character(len=80)   :: chpar
#ifdef FT_HAS_CWAVE
    real(dp)            :: ksn_max
    real(dp), parameter :: shmax = asinh(huge(1.0_dp)) ! limit on sinh argument
#endif

    !! --- Logic section ---

    write(chpar,600) H,T,eps,D
    call ffamsg_list ('  -> Computing 5th order Stokes wave coefficients')
    call ffamsg_list (trim(chpar))
    if (d < H) then
       ierr = -1
       call ffamsg_list ('Error :   Invalid water depth, D < H')
       goto 990
    end if

    RFUNC(3) = eps
    RFUNC(2) = twoPi/T
    RFUNC(1) = waveNumber(RFUNC(2),g,D)
    write(chpar,601) RFUNC(1)
    call ffamsg_list (' First order '//trim(chpar))
#ifdef FT_HAS_CWAVE
    call stokes5 (g,D,T,H,RFUNC(1),RFUNC(9),RFUNC(4),ierr)
    write(chpar,601) RFUNC(1)
    call ffamsg_list (' Fifth order '//trim(chpar))

    ksn_max = 5.0_dp*RFUNC(1)*(D+sum(RFUNC(4:8)))

    if (ierr > 0) then
       ierr = -ierr
    else if (ksn_max > shmax .or. isNaN(ksn_max)) then
       ierr = -1
       call ffamsg_list ('Error :   The specified water depth (D) is too high')
       call ffamsg_list ('          for the 5th order Stokes wave formulation.')
    else
       ierr = 6 ! = (size(rfunc)-1)/2
    end if
#else
    call ffamsg_list ('Error :   Built without Stokes 5th order support.')
    RFUNC(4:13) = 0.0_dp
    ierr = -99
#endif

600 format(5X,'H =',F6.2,'  T =',F6.2,'  eps =',F6.2,'  d =',F6.2)
601 format(5X,'k =',1PE12.5)

#ifdef FT_DEBUG
    write(6,"('Stokes5: A = ',1P5E13.5)") RFUNC(9:13)
    write(6,"('Stokes5: B = ',1P5E13.5)") RFUNC(4:8)
    call flush(6)
#endif

990 if (ierr < 0) call ffamsg_list ('          initFUNC7, IERR =',IERR)

  end subroutine initFUNC7


  subroutine stokes5Wave (RFUNC,d,x,z,t,atSurface,h,u,w,du,dw)

    !!==========================================================================
    !! Evaluates the wave profile and the associated particle velocity
    !! and acceleration at a given spatial point {x,0,z} and time, t.
    !! Assuming 5nd order Stokes regular wave theory.
    !!
    !! Based on code from j.r.chaplin@soton.ac.uk
    !! See http://www.civil.soton.ac.uk/hydraulics/download/downloadtable.htm
    !! Note: This web-page no longer exists (January 2015).
    !!
    !! RFUNC  - Wave data, output from initFUNC7
    !! d      - Water depth
    !! x, z   - Current position (z=0 mean free surface level, positive upwards)
    !! t      - Current time
    !! h      - Current wave height above z=0 (depends on x and t only)
    !! u, w   - Current particle velocity in local x and z directions
    !! du, dw - Current particle acceleration in local x and z directions
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 23 May 2011 / 1.0
    !!==========================================================================

    real(dp), intent(in)  :: RFUNC(:), d, x, z, t
    logical , intent(in)  :: atSurface
    real(dp), intent(out) :: h, u, w, du, dw

    !! Local variables
    integer  :: i
    real(dp) :: k, omega, theta, ks, c, ch, s, sh, p, q

    !! --- Logic section ---

    h  = 0.0_dp
    u  = 0.0_dp
    w  = 0.0_dp
    du = 0.0_dp
    dw = 0.0_dp

    k     = RFUNC(1)
    omega = RFUNC(2)
    theta = RFUNC(3) + k*x - omega*t
    do i = 1, 5
       h = h + RFUNC(3+i)*cos(real(i,dp)*theta)
    end do

    if (atSurface) then ! z = h
       ks = k*(d+h)
    else if (z < -d .or. z > h) then
       return ! we are below the bottom or above the surface
    else
       ks = k*(d+z)
    end if

    p = 0.0_dp
    q = 0.0_dp
    do i = 1, 5
       c = cos(real(i,dp)*theta)
       s = sin(real(i,dp)*theta)
       ch = cosh(real(i,dp)*ks)
       sh = sinh(real(i,dp)*ks)
       u = u + RFUNC(8+i)*ch*c
       w = w + RFUNC(8+i)*sh*s
       p = p + RFUNC(8+i)*ch*s
       q = q + RFUNC(8+i)*sh*c
    end do

    du =  omega*p - u*k*p + w*k*q
    dw = -omega*q + u*k*q + w*k*p

  end subroutine stokes5Wave


  subroutine initFUNC8 (H,T,eps,g,D,RFUNC,iOrder)

    !!==========================================================================
    !! Solves the streamline wave equation. The subroutine cw260 is (a slightly
    !! modified version of) the code that was downloaded from this web-page:
    !! http://www.civil.soton.ac.uk/hydraulics/download/downloadtable.htm
    !! Note: This web-page no longer exists (January 2015).
    !!
    !! H   - Wave height
    !! T   - Wave period
    !! eps - Phase shift
    !! g   - Gravity constant
    !! D   - Water depth
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 09 May 2011 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    real(dp), intent(in)  :: H, T, eps, g, D
    real(dp), intent(out) :: RFUNC(*)
    integer , intent(out) :: iOrder

    !! Local variables
#ifdef FT_HAS_CWAVE
#ifdef FT_DEBUG
    integer , parameter :: nverb = 1
#else
    integer , parameter :: nverb = 0
#endif
    integer , parameter :: nmax = 25
    real(dp), parameter :: shmax = asinh(huge(1.0_dp)) ! limit on sinh argument
    real(dp)            :: lambda, ksn_max, amp(nmax), c(nmax)
#endif
#ifdef FT_DEBUG
    integer             :: i
#endif
    character(len=80)   :: chpar

    !! --- Logic section ---

    write(chpar,600) H,T,eps,D
    call ffamsg_list ('  -> Solving the streamline wave equation')
    call ffamsg_list (trim(chpar))
    if (d < H) then
       iOrder = -1
       call ffamsg_list ('Error :   Invalid water depth, D < H')
       goto 990
    end if

    RFUNC(3) = eps
    RFUNC(2) = twoPi/T
    RFUNC(1) = waveNumber(RFUNC(2),g,D)
#ifdef FT_HAS_CWAVE
    call cw260 (g,D,T,H,0.0_dp,nverb,iOrder,lambda,amp,c)
    if (iOrder < 2 .or. iOrder > 25) then
       iOrder = min(-1,-iOrder)
       goto 990
    end if

    rfunc(1) = twoPi/lambda
    rfunc(4:2+iOrder) = amp(1:iOrder-1)
    rfunc(3+iOrder:1+2*iOrder) = c(2:iOrder)
    write(chpar,610) rfunc(1),lambda,iOrder
    call ffamsg_list (trim(chpar))

    ksn_max = real(iOrder-1,dp)*rfunc(1)*(D+sum(RFUNC(4:2+iOrder)))
    if (ksn_max > shmax .or. isNaN(ksn_max)) then
       iOrder = -1
       call ffamsg_list ('Error :   The specified water depth (D) is too high')
       call ffamsg_list ('          for the streamline wave formulation.')
    end if
#else
    call ffamsg_list ('Error :   Built without streamline wave support.')
    RFUNC(4:8) = 0.0_dp
    iOrder = -99
#endif

600 format(5X,'H =',F6.2,'  T =',F6.2,'  eps =',F6.2,'  d =',F6.2)
#ifdef FT_HAS_CWAVE
610 format(5X,'k =',1PE12.5,'  lambda =',E12.5,'  Order =',I3)
#endif

#ifdef FT_DEBUG
    write(6,"('#  i     amp_i      c_(i+1)')")
    write(6,"(I4,1P2E13.5)") (i,rfunc(3+i),rfunc(2+iOrder+i),i=1,iOrder-1)
    call flush(6)
#endif

990 if (iOrder < 0) call ffamsg_list ('          initFUNC8, IERR =',iOrder)

  end subroutine initFUNC8


  subroutine streamWave (RFUNC,d,x,z,t,atSurface,h,u,w,du,dw)

    !!==========================================================================
    !! Evaluates the wave profile and the associated particle velocity
    !! and acceleration at a given spatial point {x,0,z} and time, t.
    !! Assuming nonlinear streamline wave theory.
    !!
    !! Based on code from j.r.chaplin@soton.ac.uk
    !! See http://www.civil.soton.ac.uk/hydraulics/download/downloadtable.htm
    !! Note: This web-page no longer exists (January 2015).
    !!
    !! RFUNC  - Wave data, output from initFUNC8
    !! d      - Water depth
    !! x, z   - Current position (z=0 mean free surface level, positive upwards)
    !! t      - Current time
    !! h      - Current wave height above z=0 (depends on x and t only)
    !! u, w   - Current particle velocity in local x and z directions
    !! du, dw - Current particle acceleration in local x and z directions
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 09 May 2011 / 1.0
    !!==========================================================================

    real(dp), intent(in)  :: RFUNC(:), d, x, z, t
    logical , intent(in)  :: atSurface
    real(dp), intent(out) :: h, u, w, du, dw

    !! Local variables
    integer  :: i, nf
    real(dp) :: s1, s2, s3, s4, k, ks, omega, theta
    real(dp) :: ce, ch, sh, cs, sn, ut, wt, ux, wx, uz, wz

    !! --- Logic section ---

    h  = 0.0_dp
    u  = 0.0_dp
    w  = 0.0_dp
    du = 0.0_dp
    dw = 0.0_dp
    nf = (size(RFUNC) - 3)/2

    k     = RFUNC(1)
    omega = RFUNC(2)
    theta = RFUNC(3) + k*x - omega*t

    do i = 1, nf
       h = h + cos(i*theta)*RFUNC(3+i)
    end do

    if (atSurface) then ! z = h
       ks = k*(d+h)
    else if (z < -d .or. z > h) then
       return ! we are below the bottom or above the surface
    else
       ks = k*(d+z)
    end if

    s1 = 0.0_dp
    s2 = 0.0_dp
    s3 = 0.0_dp
    s4 = 0.0_dp

    do i = 1, nf
       ce = RFUNC(3+nf+i)
       ch = cosh(i*ks)
       sh = sinh(i*ks)
       cs = cos(i*theta)
       sn = sin(i*theta)
       s1 = s1 + i*ch*cs*ce
       s2 = s2 + i*sh*sn*ce
       s3 = s3 + i*i*ch*sn*ce
       s4 = s4 + i*i*sh*cs*ce
    end do

    u  =  k*s1
    w  =  k*s2
    ut =  k*omega*s3
    wt = -k*omega*s4
    ux = -k*k*s3
    wx =  k*k*s4
    uz =  wx
    wz = -ux
    du =  ut + u*ux + w*uz
    dw =  wt + u*wx + w*wz

  end subroutine streamWave


  subroutine initFUNC9 (IOP,NR,NC,H3,T1,Gamma,Omega0,dOmega,g,D,ramp,embSLW, &
       &                IFUNC,RFUNC,IERR,RSEED)

    !!==========================================================================
    !! Initializes an irregular wave function based on statistical parameters,
    !! with embedded streamline waves at specified time locations.
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 16 May 2012 / 1.0
    !!==========================================================================

    use FFaMsgInterface, only : ffamsg_list

    integer ,           intent(in)  :: IOP, NR, NC
    real(dp),           intent(in)  :: H3, T1, Gamma, Omega0, dOmega, g, D
    real(dp),           intent(in)  :: ramp(2), embSLW(:,:)
    integer ,           intent(out) :: IFUNC(:)
    real(dp),           intent(out) :: RFUNC(:)
    integer ,           intent(out) :: IERR
    integer , optional, intent(in)  :: RSEED

    !! Local variables
    integer  :: i, iStw, nStw
    real(dp) :: SFUNC(64)
    real(dp), pointer :: irrw(:,:)

    !! --- Logic section ---

    if (IOP > 0) then
       irrw => toMatrix(RFUNC(1:NR*NC),NR,NC)
       if (IOP > 4) then
          call initFUNC4 (H3,T1,Gamma,Omega0,dOmega,g,D,irrw,IERR,RSEED)
       else
          call initFUNC4 (IOP,H3,T1,Gamma,Omega0,dOmega,g,D,irrw,IERR,RSEED)
       end if
       if (IERR < 0) goto 990
    end if

    IERR = size(IFUNC) - size(embSLW,2) - 4
    if (IERR < 0) goto 990

    if (NR == 3) then
       iStw = 3*NC + 3 ! Make space for the gravity constant
       RFUNC(iStw-2) = g
    else
       iStw = NR*NC + 2
    end if

    IFUNC(3) = NC ! Number of irregular wave components
    IFUNC(4) = NR ! Number of data per irregular wave component
    RFUNC(iStw-1:iStw) = ramp
    do i = 1, size(embSLW,2)
       call initFUNC8 (embSLW(3,i),embSLW(2,i),0.0_dp,g,D,SFUNC,IERR)
       if (IERR < 0) goto 990
       IFUNC(4+i) = IERR-1 ! Order of streamline wave component
       nStw = 1+2*IERR
       IERR = size(RFUNC) - (iStw+2+nStw)
       if (IERR < 0) goto 990
       RFUNC(iStw+1:iStw+3)      = embSLW(1:3,i) ! T0, Tp, H for the streamline
       RFUNC(iStw+4:iStw+3+nStw) = SFUNC(1:nStw) ! Streamline wave coefficients
       iStw = iStw + 3+nStw
    end do

    IERR = iStw-1 ! = size(RFUNC)
    return

990 call ffamsg_list ('          initFUNC9, IERR =',IERR)

  contains

    function toMatrix (array,nr,nc)
      integer , intent(in)         :: nr, nc
      real(dp), intent(in), target :: array(nr,nc)
      real(dp), pointer            :: toMatrix(:,:)
      toMatrix => array
    end function toMatrix

  end subroutine initFUNC9


  subroutine embeddedWave (IFUNC,RFUNC,g,d,x,z,t,atSurf,noWS,h,u,w,du,dw)

    !!==========================================================================
    !! Evaluates the wave profile and the associated particle velocity
    !! and acceleration at a given spatial point {x,0,z} and time, t.
    !! Assuming finite depth water waves, linear wave theory, with embedded
    !! nonlinear streamline waves at user-defined times.
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 16 May 2012 / 1.0
    !!==========================================================================

    integer , intent(in)  :: IFUNC(:)
    real(dp), intent(in)  :: RFUNC(:), g, d, x, z, t
    logical , intent(in)  :: atSurf, noWS
    real(dp), intent(out) :: h, u, w, du, dw

    !! Local variables
    integer  :: i, ipSt, M, N, nSt
    logical  :: irrW
    real(dp) :: Ts, Tp, cdt, scL, scS, hs, us, ws, dus, dws, startBl, stopBl

    !! --- Logic section ---

    N = IFUNC(3)
    M = IFUNC(4)
    if (M == 3) then
       ipSt = 4 + 3*N
    else
       ipSt = 3 + M*N
    end if
    nSt = 4
    stopBl = RFUNC(ipSt-2)
    startBl = RFUNC(ipSt-1)

    !! Check if we are within any of the streamline wave function domains
    Ts = 0.0_dp
    Tp = 0.0_dp
    irrW = .true.
    do i = 5, size(IFUNC)
       nSt = 6 + 2*IFUNC(i)
       if (ipSt+1 <= size(RFUNC)) then
          Ts = T-RFUNC(ipSt)
          Tp = RFUNC(ipSt+1)
          if (abs(Ts) <= startBl*Tp) then
             irrW = abs(Ts) > stopBl*Tp
             exit
          end if
       end if
       ipSt = ipSt + nSt
    end do

    if (irrW) then
       !! Evaluate the irregular wave
       call finiteDepthWave (reshape(RFUNC(1:M*N),(/M,N/)), &
            &                g,d,x,z,t,atSurf,noWS,h,u,w,du,dw)
    else
       !! Evaluate the streamline wave function (no blending)
       call streamWave (RFUNC(ipSt+3:ipSt+nSt-1),d,x,z,Ts,atSurf,h,u,w,du,dw)
    end if
    if (.not. irrW .or. i > size(IFUNC)) return

    !! We are in the transition zone where the two wave formulations
    !! are to be blended, evaluate the smooth transfer functions
    cdt = cos(pi*(abs(Ts)/Tp-stopBl)/(startBl-stopBl))
    scS = 0.5_dp*(1.0_dp+cdt)
    scL = 0.5_dp*(1.0_dp-cdt)

    !! Evaluate the streamline function to blend with the irregular wave
    call streamWave (RFUNC(ipSt+3:ipSt+nSt-1),d,x,z,Ts,atSurf,hs,us,ws,dus,dws)

    !! Compute the blended wave kinematics
    h  =  h*scL +  hs*scS
    u  =  u*scL +  us*scS
    w  =  w*scL +  ws*scS
    du = du*scL + dus*scS
    dw = dw*scL + dws*scS

  end subroutine embeddedWave


  subroutine userDefinedWave (ikf,IFUNC,RFUNC,g,d,Xt,atSurf,h,u,du,ierr)

    !!==========================================================================
    !! Evaluates the user-defined wave profile and the associated particle
    !! velocity and acceleration at a given spatial point and time.
    !!
    !! Programmer : Knut Morten Okstad              date/rev : 22 Jun 2016 / 1.0
    !!==========================================================================

    use FFaUserFuncInterface, only : ffauf_wave
    use FFaMsgInterface     , only : ffamsg_list

    integer , intent(in)  :: ikf, IFUNC(3)
    real(dp), intent(in)  :: RFUNC(:), g, d, Xt(4)
    logical , intent(in)  :: atSurf
    real(dp), intent(out) :: h, u(3), du(3)
    integer , intent(out) :: ierr

    !! Local variables
    integer :: funcId

    !! --- Logic section ---

    funcId = iFunc(3)
    if (atSurf) then
       ierr = ffauf_wave(-ikf,funcId,d,g,rFunc,Xt,h,u,du)
    else
       ierr = ffauf_wave(ikf,funcId,d,g,rFunc,Xt,h,u,du)
    end if
    if (ierr == 0) return

    call ffamsg_list ('Error :   Could not get wave from user function',funcId)
    call ffamsg_list ('          userDefinedWave, baseId =',ikf)

  end subroutine userDefinedWave

end module WaveFunctionsModule
