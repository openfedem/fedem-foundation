// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_FOURIER_H
#define FFP_FOURIER_H

#include <vector>


namespace FFpFourier
{
  bool FFT(const std::vector<double>& xRe, const std::vector<double>& xIm,
           std::vector<double>& yRe, std::vector<double>& yIm);

  int getMaxPrimeFactor();
}

#endif
