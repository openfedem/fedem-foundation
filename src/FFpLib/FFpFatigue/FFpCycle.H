// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_CYCLE_H
#define FFP_CYCLE_H

#include <iostream>


class FFpCycle
{
  static double toMPaScale;
  double first, second;

public:
  FFpCycle(double a = 0.0, double b = 0.0) { first = a; second = b; }

  static void setScaleToMPa(double scale) { toMPaScale = scale; }

  double mean() const;
  double range() const;

  friend bool operator<(const FFpCycle& lhs, const FFpCycle& rhs);
  friend std::ostream& operator<<(std::ostream& s, const FFpCycle& obj);
};

#endif
