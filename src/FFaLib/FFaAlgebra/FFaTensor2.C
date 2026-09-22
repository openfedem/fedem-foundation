// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaTensor2.C
  \brief 2nd order symmetric tensors in 2D space.
*/

#include "FFaLib/FFaAlgebra/FFaTensor2.H"
#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaAlgebra/FFaTensor1.H"
#include "FFaLib/FFaAlgebra/FFaMat33.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaAlgebra/FFaTensorTransforms.H"


FFaTensor2::FFaTensor2(const FFaTensor3& t)
{
  myT[0] = t[0];
  myT[1] = t[1];
  myT[2] = t[3];
}


FFaTensor2::FFaTensor2(const FFaTensor1& t)
{
  myT[0] = t;
  myT[1] = myT[2] = 0.0;
}


FFaTensor2& FFaTensor2::operator= (const FFaTensor3& t)
{
  myT[0] = t[0];
  myT[1] = t[1];
  myT[2] = t[3];
  return *this;
}


FFaTensor2& FFaTensor2::operator= (const FFaTensor1& t)
{
  myT[0] = t;
  myT[1] = myT[2] = 0.0;
  return *this;
}


FFaTensor2& FFaTensor2::rotate(const double ex[2], const double ey[2])
{
  FFaTensorTransforms::rotate(myT.data(), ex,ey, myT.data());
  return *this;
}


double FFaTensor2::vonMises() const
{
  return FFaTensorTransforms::vonMises(myT[0],myT[1],myT[2]);
}


/*!
  HUGE_VAL will be returned if the max shear value can not be found.
*/

double FFaTensor2::maxShear() const
{
  double princVal[3] = { 0.0, 0.0, 0.0 };
  if (!FFaTensorTransforms::principalVals2D(myT[0],myT[1],myT[2],princVal))
    return HUGE_VAL;

  return FFaTensorTransforms::maxShearValue(princVal[0],princVal[1]);
}


/*!
  The vector is set to zero if the max shear directions can not be found.
*/

void FFaTensor2::maxShear(FaVec3& v) const
{
  double values[2], max[2], min[2];
  if (FFaTensorTransforms::principalDirs(myT.data(),values,max,min) == 0)
  {
    v[2] = 0.0;
    FFaTensorTransforms::maxShearDir(2,max,min,v.getPt());
    v *= FFaTensorTransforms::maxShearValue(values[0],values[1]);
  }
  else
    v.clear();
}


/*!
  HUGE_VAL will be returned if the principal value can not be found.
*/

double FFaTensor2::maxPrinsipal(bool absMax) const
{
  double princVal[3] = { 0.0, 0.0, 0.0 };
  if (!FFaTensorTransforms::principalVals2D(myT[0],myT[1],myT[2],princVal))
    return HUGE_VAL;

  return princVal[absMax && fabs(princVal[1]) > fabs(princVal[0]) ? 1 : 0];
}


/*!
  HUGE_VAL will be returned if the principal value can not be found.
*/

double FFaTensor2::minPrinsipal() const
{
  double princVal[3] = { 0.0, 0.0, 0.0 };
  if (!FFaTensorTransforms::principalVals2D(myT[0],myT[1],myT[2],princVal))
    return HUGE_VAL;

  return princVal[1];
}


/*!
  HUGE_VAL will be returned if the principal values can not be found.
*/

void FFaTensor2::prinsipalValues(double& max, double& min) const
{
  double princVal[3] = { 0.0, 0.0, 0.0 };
  if (!FFaTensorTransforms::principalVals2D(myT[0],myT[1],myT[2],princVal))
    max = min = HUGE_VAL;
  else
  {
    max = princVal[0];
    min = princVal[1];
  }
}


/*!
  A valid rotation matrix corresponding to the principal axes of the tensor
  is calculated. The associated principal values are also found in the
  corresponding order. If the rotation matrix can't be found,
  the identity will be returned, along with a vector of HUGE_VAL.
*/

void FFaTensor2::prinsipalValues(FaVec3& values, FaMat33& rotation) const
{
  values[2] = 0.0;
  rotation.setIdentity();
  if (FFaTensorTransforms::principalDirs(myT.data(),
                                         values.getPt(),
                                         rotation[0].getPt(),
                                         rotation[1].getPt()))
  {
    values[0] = values[1] = HUGE_VAL;
    values[2] = 0.0;
  }
  else if ((rotation[0] ^ rotation[1]).isParallell(rotation[2]) != 1)
  {
    // Swap the 1st and 2nd
    std::swap(rotation[0],rotation[1]);
    std::swap(values[0],values[1]);
  }
}


///////////////////
// Global operators
///////////////////
//! \cond DO_NOT_DOCUMENT

FFaTensor2 operator- (const FFaTensor2& t)
{
  return FFaTensor2(-t.myT[0], -t.myT[1], -t.myT[2]);
}


FFaTensor2 operator+ (const FFaTensor2& a, const FFaTensor2& b)
{
  return FFaTensor2(a.myT[0] + b.myT[0],
                    a.myT[1] + b.myT[1],
                    a.myT[2] + b.myT[2]);
}

FFaTensor2 operator- (const FFaTensor2& a, const FFaTensor2& b)
{
  return FFaTensor2(a.myT[0] - b.myT[0],
                    a.myT[1] - b.myT[1],
                    a.myT[2] - b.myT[2]);
}


FFaTensor2 operator* (const FFaTensor2& a, double d)
{
  return FFaTensor2(a.myT[0]*d, a.myT[1]*d, a.myT[2]*d);
}

FFaTensor2 operator* (double d, const FFaTensor2& a)
{
  return FFaTensor2(a.myT[0]*d, a.myT[1]*d, a.myT[2]*d);
}


FFaTensor2 operator/ (const FFaTensor2& a, double d)
{
  if (fabs(d) < 1.0e-16)
    return FFaTensor2(HUGE_VAL);

  return FFaTensor2(a.myT[0]/d, a.myT[1]/d, a.myT[2]/d);
}


bool operator== (const FFaTensor2& a, const FFaTensor2& b)
{
  return (a.myT[0] == b.myT[0] && a.myT[1] == b.myT[1] && a.myT[2] == b.myT[2]);
}


bool operator!= (const FFaTensor2& a, const FFaTensor2& b)
{
  return !(a==b);
}


FFaTensor3 operator* (const FFaTensor2& a, const FaMat33 & m)
{
  FFaTensor3 result;
  FFaTensor3 in(a);
  FFaTensorTransforms::rotate(in.getPt(),
                              m[0].getPt(), m[1].getPt(), m[2].getPt(),
                              result.getPt());
  return result;
}

FFaTensor3 operator* (const FFaTensor2& a, const FaMat34& m)
{
  FFaTensor3 result;
  FFaTensor3 in(a);
  FFaTensorTransforms::rotate(in.getPt(),
                              m[0].getPt(), m[1].getPt(), m[2].getPt(),
                              result.getPt());
  return result;
}


FFaTensor3 operator* (const FaMat33& m, const FFaTensor2& a)
{
  return a*m;
}

FFaTensor3 operator* (const FaMat34& m, const FFaTensor2& a)
{
  return a*m;
}


std::ostream& operator<< (std::ostream& s, const FFaTensor2& t)
{
  s << t.myT[0] << ' ' << t.myT[1] << ' ' << t.myT[2];
  return s;
}

std::istream& operator>> (std::istream& s, FFaTensor2& t)
{
  FFaTensor2 tmpT;
  s >> tmpT[0] >> tmpT[1] >> tmpT[2];
  if (s) t = tmpT;
  return s;
}

//! \endcond
