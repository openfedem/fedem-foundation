// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_FOURIER_H
#define FFP_FOURIER_H

#include <vector>


class FFpFourier
{
public:
  static bool FFT (const std::vector<double>& xRe,
                   const std::vector<double>& xIm,
		   std::vector<double>& yRe, std::vector<double>& yIm);

  static int getMaxPrimeFactor ();

private:
  static bool transTableSetup (int* sofar, int* actual, int* remain,
			       int& nFact, int nPoints);

  static void factorize (int n, int& nFact, int* fact);

  static void permute (int nFact, int* fact, int* remain,
		       const std::vector<double>& xRe,
		       const std::vector<double>& xIm,
		       std::vector<double>& yRe,
		       std::vector<double>& yIm);

  static void twiddleTransf (int sofarRadix, int radix, int remainRadix,
			     std::vector<double>& yRe,
			     std::vector<double>& yIm);
  static void initTrig (int radix);
  static void fft_4 (double* aRe, double* aIm);
  static void fft_5 (double* aRe, double* aIm);
  static void fft_8 ();
  static void fft_10 ();
  static void fft_odd (int radix);
};

#endif
