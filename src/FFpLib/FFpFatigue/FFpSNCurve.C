// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cmath>

#include "FFpSNCurve.H"



FFpSNCurveNorSok::FFpSNCurveNorSok(double log_a1, double log_a2,
				   double m_1, double m_2)
{
  loga.push_back(log_a1);
  loga.push_back(log_a2);
  m.push_back(m_1);
  m.push_back(m_2);

  // Calculate the intersection between the two lines
  logN0.push_back((m_2*log_a1 - m_1*log_a2)/(m_2 - m_1));
}


double FFpSNCurveNorSok::getValue(double s) const
{
  double logN;
  size_t size = logN0.size();
  for (size_t i = 0; i < size; i++)
  {
    logN = loga[i] - m[i]*log10(s);
    if (logN < logN0[i])
      return pow(10.0, logN);
  }

  logN = loga[size] - m[size]*log10(s);
  return pow(10.0, logN);
}


double FFpSNCurveBritish::getValue(double s) const
{
  double logN = loga[0] - loga[1]*m[1] - m[0]*log10(s);
  return pow(10.0, logN);
}
