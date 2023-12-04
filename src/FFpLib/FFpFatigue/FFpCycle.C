// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include "FFpCycle.H"


double FFpCycle::toMPaScale = 1.0;


double FFpCycle::mean() const
{
  return 0.5*(first+second)*toMPaScale;
}


double FFpCycle::range() const
{
  return fabs(first-second)*toMPaScale;
}


std::ostream& operator<<(std::ostream& s, const FFpCycle& obj)
{
  return s << obj.first <<" "<< obj.second;
}


bool operator<(const FFpCycle& lhs, const FFpCycle& rhs)
{
  return lhs.range() < rhs.range();
}
