// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpCurveData/FFpFourier.H"
#include <string.h>
#include <math.h>

/************************************************************************

  FFT (const std::vector<double>& xRe, const std::vector<double>& xIm,
             std::vector<double>& yRe,       std::vector<double>& yIm)
 ------------------------------------------------------------------------
  Input:
      xRe  real part of input sequence.
      xIm  imaginary part of input sequence.
  Output:
      yRe  real part of output sequence.
      yIm  imaginary part of output sequence.
 ------------------------------------------------------------------------
  Function:
      The procedure performs a fast discrete Fourier transform (FFT) of
      a complex sequence, x, of an arbitrary length, n. The output, y,
      is also a complex sequence of length n.

      y[k] = sum(x[m]*exp(-i*2*pi*k*m/n), m=0..(n-1)), k=0,...,(n-1)

      The largest prime factor of n must be less than or equal to the
      constant maxPrimeFactor defined below.
 ------------------------------------------------------------------------
  Implementation notes:
      The general idea is to factor the length of the DFT, n, into
      factors that are efficiently handled by the routines.

      A number of short DFT's are implemented with a minimum of
      arithmetical operations and using (almost) straight line code
      resulting in very fast execution when the factors of n belong
      to this set. Especially radix-10 is optimized.

      Prime factors, that are not in the set of short DFT's are handled
      with direct evaluation of the DFP expression.
 ------------------------------------------------------------------------
  The following procedures are used :
      factorize       :  factor the transformation length.
      transTableSetup :  setup table with sofar-, actual-, and remainRadix.
      permute         :  permutation allows in-place calculations.
      twiddleTransf   :  twiddle multiplications and DFT's for one stage.
      initTrig        :  initialise sine/cosine table.
      fft_4           :  length 4 DFT, a la Nussbaumer.
      fft_5           :  length 5 DFT, a la Nussbaumer.
      fft_10          :  length 10 DFT using prime factor FFT.
      fft_odd         :  length n DFT, n odd.
*************************************************************************/

#define maxFactorCount     20
#define maxPrimeFactor     1009
#define maxPrimeFactorDiv2 505

static double pi = 3.14159265358979323846;

static double c3_1 = -1.5;              //  = cos(2*pi/3)-1
static double c3_2 =  0.86602540378444; //  = sin(2*pi/3)
static double c5_1 = -1.25;             //  = (cos(2*pi/5)+cos(4*pi/5))/2-1
static double c5_2 =  0.55901699437495; //  = (cos(2*pi/5)-cos(4*pi/5))/2
static double c5_3 = -0.95105651629515; //  = -sin(2*pi/5)
static double c5_4 = -1.5388417685876;  //  = -(sin(2*pi/5)+sin(4*pi/5))
static double c5_5 =  0.36327126400268; //  = (sin(2*pi/5)-sin(4*pi/5))
static double c8   =  0.70710678118655; //  = 1/sqrt(2)

static double twiddleRe[maxPrimeFactor], twiddleIm[maxPrimeFactor],
              trigRe[maxPrimeFactor], trigIm[maxPrimeFactor],
              zRe[maxPrimeFactor], zIm[maxPrimeFactor];
static double vRe[maxPrimeFactorDiv2], vIm[maxPrimeFactorDiv2];
static double wRe[maxPrimeFactorDiv2], wIm[maxPrimeFactorDiv2];


bool FFpFourier::FFT (const std::vector<double>& xRe, const std::vector<double>& xIm,
		      std::vector<double>& yRe, std::vector<double>& yIm)
{
  int sofarRadix[maxFactorCount];
  int actualRadix[maxFactorCount];
  int remainRadix[maxFactorCount];
  int nFact;

  if (!transTableSetup(sofarRadix, actualRadix, remainRadix, nFact, xRe.size()))
    return false;

  permute(nFact, actualRadix, remainRadix, xRe, xIm, yRe, yIm);
  for (int i = 1; i <= nFact; i++)
    twiddleTransf(sofarRadix[i], actualRadix[i], remainRadix[i], yRe, yIm);

  return true;
}


int FFpFourier::getMaxPrimeFactor()
{
  return maxPrimeFactor;
}


/****************************************************************************
  After N is factored the parameters that control the stages are generated.
  For each stage we have:
    sofar   : the product of the radices so far.
    actual  : the radix handled in this stage.
    remain  : the product of the remaining radices.
 ****************************************************************************/

bool FFpFourier::transTableSetup(int* sofar, int* actual, int* remain,
				 int& nFact, int nPoints)
{
  factorize(nPoints, nFact, actual);
  if (actual[1] > maxPrimeFactor) return false;

  remain[0]=nPoints;
  sofar[1]=1;
  remain[1]=nPoints / actual[1];
  for (int i=2; i<=nFact; i++)
  {
    sofar[i]=sofar[i-1]*actual[i-1];
    remain[i]=remain[i-1] / actual[i];
  }
  return true;
}

void FFpFourier::factorize(int n, int& nFact, int* fact)
{
  int i,j,k;
  int nRadix;
  int radices[7];
  int factors[maxFactorCount];

  nRadix    =  6;
  radices[1]=  2;
  radices[2]=  3;
  radices[3]=  4;
  radices[4]=  5;
  radices[5]=  8;
  radices[6]= 10;

  if (n==1)
  {
    j=1;
    factors[1]=1;
  }
  else
  {
    j=0;
    factors[0]=0;
  }

  i=nRadix;
  while (n>1 && i>0)
    if (n%radices[i] == 0)
    {
      n/=radices[i];
      factors[++j]=radices[i];
    }
    else
      i--;

  if (factors[j] == 2) // substitute factors 2*8 with 4*4
  {
    i = j-1;
    while (i>0 && factors[i] != 8) i--;
    if (i>0)
    {
      factors[j] = 4;
      factors[i] = 4;
    }
  }
  if (n>1)
  {
    for (k=2; k<sqrt((float)n)+1; k++)
      while (n%k == 0)
      {
	n/=k;
	factors[++j]=k;
      }
    if (n>1)
      factors[++j]=n;
  }

  for (i=1; i<=j; i++)
    fact[i] = factors[j-i+1];

  nFact=j;
}


/****************************************************************************
  The sequence y is the permuted input sequence x so that the following
  transformations can be performed in-place, and the final result is the
  normal order.
 ****************************************************************************/

void FFpFourier::permute(int /*nFact*/, int* fact, int* remain,
			 const std::vector<double>& xRe, const std::vector<double>& xIm,
			 std::vector<double>& yRe, std::vector<double>& yIm)
{
  int i, j, k, lastPoint = xRe.size()-1;
  bool isComplex = xIm.size() >= xRe.size();

  int count[maxFactorCount];
  memset(count,0,maxFactorCount*sizeof(int));

  yRe.resize(xRe.size());
  yIm.resize(xRe.size());
  for (i = k = 0; i < lastPoint; i++)
  {
    yRe[i] = xRe[k];
    yIm[i] = isComplex ? xIm[k] : 0.0;

    k+=remain[1];
    count[1]++;
    for (j = 1; count[j] >= fact[j]; j++)
    {
      count[j]=0;
      count[j+1]++;
      k+=remain[j+1]-remain[j-1];
    }
  }

  yRe[lastPoint] = xRe[lastPoint];
  yIm[lastPoint] = isComplex ? xIm[lastPoint] : 0.0;
}


/****************************************************************************
  Twiddle factor multiplications and transformations are performed on a
  group of data. The number of multiplications with 1 are reduced by skipping
  the twiddle multiplication of the first stage and of the first group of the
  following stages.
 ***************************************************************************/

void FFpFourier::twiddleTransf(int sofarRadix, int radix, int remainRadix,
			       std::vector<double>& yRe, std::vector<double>& yIm)

{
  double gem;
  double t1_re,t1_im, t2_re,t2_im, t3_re,t3_im;
  double t4_re,t4_im, t5_re,t5_im;
  double m2_re,m2_im, m3_re,m3_im, m4_re,m4_im;
  double m1_re,m1_im, m5_re,m5_im;
  double s1_re,s1_im, s2_re,s2_im, s3_re,s3_im;
  double s4_re,s4_im, s5_re,s5_im;

  initTrig(radix);

  double omega = 2*pi/(double)(sofarRadix*radix);
  double cosw  =  cos(omega);
  double sinw  = -sin(omega);
  double tw_re = 1.0;
  double tw_im = 0.0;

  int dataOffset = 0;
  int groupOffset = 0;
  int adr = 0;
  int groupNo, dataNo, blockNo, twNo;

  for (dataNo=0; dataNo<sofarRadix; dataNo++)
  {
    if (sofarRadix>1)
    {
      twiddleRe[0] = 1.0;
      twiddleIm[0] = 0.0;
      twiddleRe[1] = tw_re;
      twiddleIm[1] = tw_im;
      for (twNo=2; twNo<radix; twNo++)
      {
	twiddleRe[twNo]=tw_re*twiddleRe[twNo-1] - tw_im*twiddleIm[twNo-1];
	twiddleIm[twNo]=tw_im*twiddleRe[twNo-1] + tw_re*twiddleIm[twNo-1];
      }
      gem   = cosw*tw_re - sinw*tw_im;
      tw_im = sinw*tw_re + cosw*tw_im;
      tw_re = gem;
    }
    for (groupNo=0; groupNo<remainRadix; groupNo++)
    {
      if (sofarRadix>1 && dataNo > 0)
      {
	zRe[0]=yRe[adr];
	zIm[0]=yIm[adr];
	blockNo=1;
	do
	{
	  adr += sofarRadix;
	  zRe[blockNo]=  twiddleRe[blockNo] * yRe[adr]
	  	       - twiddleIm[blockNo] * yIm[adr];
	  zIm[blockNo]=  twiddleRe[blockNo] * yIm[adr]
	               + twiddleIm[blockNo] * yRe[adr];
	  blockNo++;
	}
	while (blockNo < radix);
      }
      else
	for (blockNo=0; blockNo<radix; blockNo++)
	{
	  zRe[blockNo]=yRe[adr];
	  zIm[blockNo]=yIm[adr];
	  adr+=sofarRadix;
	}

      switch(radix)
      {
	case  2 : gem=zRe[0] + zRe[1];
	          zRe[1]=zRe[0] -  zRe[1]; zRe[0]=gem;
		  gem=zIm[0] + zIm[1];
		  zIm[1]=zIm[0] - zIm[1]; zIm[0]=gem;
		  break;
        case  3 : t1_re=zRe[1] + zRe[2]; t1_im=zIm[1] + zIm[2];
	          zRe[0]=zRe[0] + t1_re; zIm[0]=zIm[0] + t1_im;
		  m1_re=c3_1*t1_re; m1_im=c3_1*t1_im;
		  m2_re=c3_2*(zIm[1] - zIm[2]);
		  m2_im=c3_2*(zRe[2] -  zRe[1]);
		  s1_re=zRe[0] + m1_re; s1_im=zIm[0] + m1_im;
		  zRe[1]=s1_re + m2_re; zIm[1]=s1_im + m2_im;
		  zRe[2]=s1_re - m2_re; zIm[2]=s1_im - m2_im;
		  break;
        case  4 : t1_re=zRe[0] + zRe[2]; t1_im=zIm[0] + zIm[2];
                  t2_re=zRe[1] + zRe[3]; t2_im=zIm[1] + zIm[3];
		  m2_re=zRe[0] - zRe[2]; m2_im=zIm[0] - zIm[2];
		  m3_re=zIm[1] - zIm[3]; m3_im=zRe[3] - zRe[1];
		  zRe[0]=t1_re + t2_re; zIm[0]=t1_im + t2_im;
		  zRe[2]=t1_re - t2_re; zIm[2]=t1_im - t2_im;
		  zRe[1]=m2_re + m3_re; zIm[1]=m2_im + m3_im;
		  zRe[3]=m2_re - m3_re; zIm[3]=m2_im - m3_im;
		  break;
        case  5 : t1_re=zRe[1] + zRe[4]; t1_im=zIm[1] + zIm[4];
                  t2_re=zRe[2] + zRe[3]; t2_im=zIm[2] + zIm[3];
		  t3_re=zRe[1] - zRe[4]; t3_im=zIm[1] - zIm[4];
		  t4_re=zRe[3] - zRe[2]; t4_im=zIm[3] - zIm[2];
		  t5_re=t1_re + t2_re; t5_im=t1_im + t2_im;
		  zRe[0]=zRe[0] + t5_re; zIm[0]=zIm[0] + t5_im;
		  m1_re=c5_1*t5_re; m1_im=c5_1*t5_im;
		  m2_re=c5_2*(t1_re - t2_re);
		  m2_im=c5_2*(t1_im - t2_im);
		  m3_re=-c5_3*(t3_im + t4_im);
		  m3_im=c5_3*(t3_re + t4_re);
		  m4_re=-c5_4*t4_im; m4_im=c5_4*t4_re;
		  m5_re=-c5_5*t3_im; m5_im=c5_5*t3_re;
		  s3_re=m3_re - m4_re; s3_im=m3_im - m4_im;
		  s5_re=m3_re + m5_re; s5_im=m3_im + m5_im;
		  s1_re=zRe[0] + m1_re; s1_im=zIm[0] + m1_im;
		  s2_re=s1_re + m2_re; s2_im=s1_im + m2_im;
		  s4_re=s1_re - m2_re; s4_im=s1_im - m2_im;
		  zRe[1]=s2_re + s3_re; zIm[1]=s2_im + s3_im;
		  zRe[2]=s4_re + s5_re; zIm[2]=s4_im + s5_im;
		  zRe[3]=s4_re - s5_re; zIm[3]=s4_im - s5_im;
		  zRe[4]=s2_re - s3_re; zIm[4]=s2_im - s3_im;
		  break;
        case  8 : fft_8(); break;
        case 10 : fft_10(); break;
        default : fft_odd(radix); break;
      }
      adr=groupOffset;
      for (blockNo=0; blockNo<radix; blockNo++)
      {
	yRe[adr]=zRe[blockNo]; yIm[adr]=zIm[blockNo];
	adr+=sofarRadix;
      }
      groupOffset+=sofarRadix*radix;
      adr=groupOffset;
    }
    dataOffset++;
    groupOffset=dataOffset;
    adr=groupOffset;
  }
}

void FFpFourier::initTrig(int radix)
{
  trigRe[0]=1.0; trigIm[0]=0.0;
  double w=2*pi/radix;
  double xre=cos(w);
  double xim=-sin(w);
  trigRe[1]=xre; trigIm[1]=xim;
  for (int i=2; i<radix; i++)
  {
    trigRe[i]=xre*trigRe[i-1] - xim*trigIm[i-1];
    trigIm[i]=xim*trigRe[i-1] + xre*trigIm[i-1];
  }
}

void FFpFourier::fft_4(double* aRe, double* aIm)
{
  double t1_re,t1_im, t2_re,t2_im;
  double m2_re,m2_im, m3_re,m3_im;

  t1_re=aRe[0] + aRe[2]; t1_im=aIm[0] + aIm[2];
  t2_re=aRe[1] + aRe[3]; t2_im=aIm[1] + aIm[3];

  m2_re=aRe[0] - aRe[2]; m2_im=aIm[0] - aIm[2];
  m3_re=aIm[1] - aIm[3]; m3_im=aRe[3] - aRe[1];

  aRe[0]=t1_re + t2_re; aIm[0]=t1_im + t2_im;
  aRe[2]=t1_re - t2_re; aIm[2]=t1_im - t2_im;
  aRe[1]=m2_re + m3_re; aIm[1]=m2_im + m3_im;
  aRe[3]=m2_re - m3_re; aIm[3]=m2_im - m3_im;
}

void FFpFourier::fft_5(double* aRe, double* aIm)
{
  double  t1_re,t1_im, t2_re,t2_im, t3_re,t3_im;
  double  t4_re,t4_im, t5_re,t5_im;
  double  m2_re,m2_im, m3_re,m3_im, m4_re,m4_im;
  double  m1_re,m1_im, m5_re,m5_im;
  double  s1_re,s1_im, s2_re,s2_im, s3_re,s3_im;
  double  s4_re,s4_im, s5_re,s5_im;

  t1_re=aRe[1] + aRe[4]; t1_im=aIm[1] + aIm[4];
  t2_re=aRe[2] + aRe[3]; t2_im=aIm[2] + aIm[3];
  t3_re=aRe[1] - aRe[4]; t3_im=aIm[1] - aIm[4];
  t4_re=aRe[3] - aRe[2]; t4_im=aIm[3] - aIm[2];
  t5_re=t1_re + t2_re; t5_im=t1_im + t2_im;
  aRe[0]=aRe[0] + t5_re; aIm[0]=aIm[0] + t5_im;
  m1_re=c5_1*t5_re; m1_im=c5_1*t5_im;
  m2_re=c5_2*(t1_re - t2_re); m2_im=c5_2*(t1_im - t2_im);

  m3_re=-c5_3*(t3_im + t4_im); m3_im=c5_3*(t3_re + t4_re);
  m4_re=-c5_4*t4_im; m4_im=c5_4*t4_re;
  m5_re=-c5_5*t3_im; m5_im=c5_5*t3_re;

  s3_re=m3_re - m4_re; s3_im=m3_im - m4_im;
  s5_re=m3_re + m5_re; s5_im=m3_im + m5_im;
  s1_re=aRe[0] + m1_re; s1_im=aIm[0] + m1_im;
  s2_re=s1_re + m2_re; s2_im=s1_im + m2_im;
  s4_re=s1_re - m2_re; s4_im=s1_im - m2_im;

  aRe[1]=s2_re + s3_re; aIm[1]=s2_im + s3_im;
  aRe[2]=s4_re + s5_re; aIm[2]=s4_im + s5_im;
  aRe[3]=s4_re - s5_re; aIm[3]=s4_im - s5_im;
  aRe[4]=s2_re - s3_re; aIm[4]=s2_im - s3_im;
}

void FFpFourier::fft_8()
{
  double aRe[4], aIm[4], bRe[4], bIm[4], gem;

  aRe[0] = zRe[0];    bRe[0] = zRe[1];
  aRe[1] = zRe[2];    bRe[1] = zRe[3];
  aRe[2] = zRe[4];    bRe[2] = zRe[5];
  aRe[3] = zRe[6];    bRe[3] = zRe[7];

  aIm[0] = zIm[0];    bIm[0] = zIm[1];
  aIm[1] = zIm[2];    bIm[1] = zIm[3];
  aIm[2] = zIm[4];    bIm[2] = zIm[5];
  aIm[3] = zIm[6];    bIm[3] = zIm[7];

  fft_4(aRe, aIm); fft_4(bRe, bIm);

  gem    = c8*(bRe[1] + bIm[1]);
  bIm[1] = c8*(bIm[1] - bRe[1]);
  bRe[1] = gem;
  gem    = bIm[2];
  bIm[2] =-bRe[2];
  bRe[2] = gem;
  gem    = c8*(bIm[3] - bRe[3]);
  bIm[3] =-c8*(bRe[3] + bIm[3]);
  bRe[3] = gem;

  zRe[0] = aRe[0] + bRe[0]; zRe[4] = aRe[0] - bRe[0];
  zRe[1] = aRe[1] + bRe[1]; zRe[5] = aRe[1] - bRe[1];
  zRe[2] = aRe[2] + bRe[2]; zRe[6] = aRe[2] - bRe[2];
  zRe[3] = aRe[3] + bRe[3]; zRe[7] = aRe[3] - bRe[3];

  zIm[0] = aIm[0] + bIm[0]; zIm[4] = aIm[0] - bIm[0];
  zIm[1] = aIm[1] + bIm[1]; zIm[5] = aIm[1] - bIm[1];
  zIm[2] = aIm[2] + bIm[2]; zIm[6] = aIm[2] - bIm[2];
  zIm[3] = aIm[3] + bIm[3]; zIm[7] = aIm[3] - bIm[3];
}

void FFpFourier::fft_10()
{
  double aRe[5], aIm[5], bRe[5], bIm[5];

  aRe[0] = zRe[0];    bRe[0] = zRe[5];
  aRe[1] = zRe[2];    bRe[1] = zRe[7];
  aRe[2] = zRe[4];    bRe[2] = zRe[9];
  aRe[3] = zRe[6];    bRe[3] = zRe[1];
  aRe[4] = zRe[8];    bRe[4] = zRe[3];

  aIm[0] = zIm[0];    bIm[0] = zIm[5];
  aIm[1] = zIm[2];    bIm[1] = zIm[7];
  aIm[2] = zIm[4];    bIm[2] = zIm[9];
  aIm[3] = zIm[6];    bIm[3] = zIm[1];
  aIm[4] = zIm[8];    bIm[4] = zIm[3];

  fft_5(aRe, aIm); fft_5(bRe, bIm);

  zRe[0] = aRe[0] + bRe[0]; zRe[5] = aRe[0] - bRe[0];
  zRe[6] = aRe[1] + bRe[1]; zRe[1] = aRe[1] - bRe[1];
  zRe[2] = aRe[2] + bRe[2]; zRe[7] = aRe[2] - bRe[2];
  zRe[8] = aRe[3] + bRe[3]; zRe[3] = aRe[3] - bRe[3];
  zRe[4] = aRe[4] + bRe[4]; zRe[9] = aRe[4] - bRe[4];

  zIm[0] = aIm[0] + bIm[0]; zIm[5] = aIm[0] - bIm[0];
  zIm[6] = aIm[1] + bIm[1]; zIm[1] = aIm[1] - bIm[1];
  zIm[2] = aIm[2] + bIm[2]; zIm[7] = aIm[2] - bIm[2];
  zIm[8] = aIm[3] + bIm[3]; zIm[3] = aIm[3] - bIm[3];
  zIm[4] = aIm[4] + bIm[4]; zIm[9] = aIm[4] - bIm[4];
}

void FFpFourier::fft_odd(int radix)
{
  double rere, reim, imre, imim;
  int    i,j,k,n,max;

  n = radix;
  max = (n + 1)/2;
  for (j=1; j < max; j++)
  {
    vRe[j] = zRe[j] + zRe[n-j];
    vIm[j] = zIm[j] - zIm[n-j];
    wRe[j] = zRe[j] - zRe[n-j];
    wIm[j] = zIm[j] + zIm[n-j];
  }

  for (j=1; j < max; j++)
  {
    zRe[j]=zRe[0];
    zIm[j]=zIm[0];
    zRe[n-j]=zRe[0];
    zIm[n-j]=zIm[0];
    k=j;
    for (i=1; i < max; i++)
    {
      rere = trigRe[k] * vRe[i];
      imim = trigIm[k] * vIm[i];
      reim = trigRe[k] * wIm[i];
      imre = trigIm[k] * wRe[i];

      zRe[n-j] += rere + imim;
      zIm[n-j] += reim - imre;
      zRe[j]   += rere - imim;
      zIm[j]   += reim + imre;

      k += j;
      if (k >= n)  k -= n;
    }
  }
  for (j=1; j < max; j++)
  {
    zRe[0]=zRe[0] + vRe[j];
    zIm[0]=zIm[0] + wIm[j];
  }
}
