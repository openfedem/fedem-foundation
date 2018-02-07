// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////
/*!
  \file FFa3PArc.C
  \brief Circular arcs in 3D space.
*/

#include "FFaLib/FFaAlgebra/FFa3PArc.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"


/*!
  \brief Static helper to calculate the center point of the arc.

  Finds the center of a circle based on two points on circle and
  vectors twowards centre at the two locations.

  This is done based on equations:
  \code
  P1 + a*P1C = C
  P2 + b*P2C = C
  \endcode

  Using x and y component equations as 4 equations with 4 unknowns.
*/

static FaVec3 findArcCenter(const FaVec3& P1, FaVec3 P1C,
                            const FaVec3& P2, FaVec3 P2C)
{
  // First find good component candidates.
  // We do not want to divide by zero and we need to use two directions.
  // That means X != Y and
  // P1C[X] != 0 and P2C[Y] != 0 and P1C[X]/P1C[Y] != P2C[X]/P2C[Y]
  // That is a result of the algebra needed to solve the equations.

  double soundness, maxSoundness = 0.0;
  int eqPermutations[6][2] = {{0,1}, {0,2}, {1,0}, {1,2}, {2,0}, {2,1}};

  int X = 0;
  int Y = 0;
  int Xc, Yc;
  P1C.normalize();
  P2C.normalize();

  for (int i = 0; i < 6; i++) {
    Xc = eqPermutations[i][0];
    Yc = eqPermutations[i][1];
    soundness = fabs(P1C[Xc] * P2C[Yc] * (P1C[Xc]/P1C[Yc] - P2C[Xc]/P2C[Yc]));
    if (soundness > maxSoundness) {
      X = Xc;
      Y = Yc;
      maxSoundness = soundness;
    }
  }

  double a = 1.0e10;
  if (maxSoundness > 1.0e-10)
    a = ((P2[X]*P2C[Y] + P1[Y]*P2C[X] - P2[Y]*P2C[X] - P1[X]*P2C[Y]) /
         (P1C[X]*P2C[Y] - P1C[Y]*P2C[X]));

  return P1 + a*P1C;
}


FaVec3 FFa3PArc::getCenter() const
{
  FaVec3 V12 = P[1] - P[0];
  FaVec3 V23 = P[2] - P[1];

  FaVec3 M12 = P[0] + 0.5*V12; // Midpoint between p1 and p2
  FaVec3 M23 = P[1] + 0.5*V23; // Midpoint betweem p2 and p3

  FaVec3 N = V12 ^ V23; // Plane normal
  FaVec3 N12 = N ^ V12; // Vector pointing twowards center
  FaVec3 N23 = N ^ V23; // Vector pointing twowards center

  return findArcCenter(M12, N12, M23, N23);
}


FaVec3 FFa3PArc::getNormal() const
{
  FaVec3 p1p2v = P[1] - P[0];
  FaVec3 p2p3v = P[2] - P[1];
  FaVec3 normal = p1p2v ^ p2p3v;
  return normal.normalize();
}


double FFa3PArc::getRadius() const
{
  return (P.front() - this->getCenter()).length();
}


bool FFa3PArc::isArc(double epsilon) const
{
  FaVec3 p1p2 = P[1] - P[0];
  if (p1p2.isZero(epsilon)) return false;

  FaVec3 p2p3 = P[2] - P[1];
  if (p2p3.isZero(epsilon)) return false;

  FaVec3 normal = p1p2.normalize() ^ p2p3.normalize();
  return normal.sqrLength() > epsilon*epsilon;
}


bool FFa3PArc::isInside(const FaVec3& point) const
{
  FaVec3 C = this->getCenter();
  return (point - C).sqrLength() <= (P.front() - C).sqrLength();
}


FaMat34 FFa3PArc::getCtrlPointMatrix(int pointNumber,
				     const FaVec3& positiveNormal,
				     bool normalIsSignOnly) const
{
  if (!this->isArc(1.0e-7))
  {
    // Probably a line
    FaVec3 Ez = P[2] - P[0];
    Ez.normalize();
    FaVec3 Ey = -positiveNormal;
    Ey.normalize();
    FaVec3 Ex = Ey ^ Ez;
    return FaMat34(Ex, Ey, Ez, P[pointNumber]);
  }
  else if (normalIsSignOnly)
  {
    FaVec3 normal = this->getNormal();
    double sign = normal*positiveNormal > 0.0 ? 1.0 : -1.0;
    FaVec3 Ex = sign * (P[pointNumber] - this->getCenter());
    Ex.normalize();
    FaVec3 Ey = -sign * normal;
    Ey.normalize();
    FaVec3 Ez = Ex ^ Ey;
    return FaMat34(Ex, Ey, Ez, P[pointNumber]);
  }
  else
  {
    FaVec3 p = P[pointNumber];
    FaVec3 c = this->getCenter();
    FaVec3 n = this->getNormal();
    FaVec3 t = (c-p) ^ n;
    FaVec3 Ez = t;
    Ez.normalize();
    FaVec3 Ey = -positiveNormal;
    Ey.normalize();
    FaVec3 Ex = Ey ^ Ez;
    return FaMat34(Ex, Ey, Ez, P[pointNumber]);
  }
}


FFa3PArc FFa3PArc::makeFromP1T1T2L(const FaVec3& p1, const FaVec3& t1,
				   const FaVec3& t2, double arcLength)
{
  // Check if it is a straight line or a full circle
  switch (t1.isParallell(t2))
    {
    case 1: // Straight line
      return FFa3PArc(p1, p1+0.5*arcLength*t1, p1+arcLength*t1);
    case -1: // Complete circle
      return FFa3PArc(p1, p1, p1);
    default:
      break;
    }

  double r = arcLength/t1.angle(t2);
  FaVec3 n = t1 ^ t2;
  n.normalize();

  FaVec3 et1 = t1; et1.normalize();
  FaVec3 et2 = t2; et2.normalize();
  FaVec3 eP1C = n ^ et1;
  FaVec3 eP2C = n ^ et2;
  FaVec3 c = p1 + r * eP1C;
  FaVec3 p2 = c - r*eP2C;
  FaVec3 M12 = p1 + 0.5*(p2-p1);
  FaVec3 pm = c + r*(M12-c).normalize();

  return FFa3PArc(p1, pm,  p2);
}


FFa3PArc FFa3PArc::makeFromTangentP1P2(const FaVec3& t, const FaVec3& p1,
                                       const FaVec3& p2, bool startTan)
{
  FaVec3 V12 = p2 - p1;
  FaVec3 M12 = p1 + 0.5*V12; // Midpoint p1 p2

  // Check if it is a straight line
  switch (t.isParallell(V12))
    {
    case 1: // Line P1 M12 P2
      return FFa3PArc(p1, M12, p2);
    case -1: // Degenerated line p1 -inf+, p2
      return FFa3PArc(p1, p1-0.5*V12, p2);
    default:
      break;
    }

  const FaVec3& p0 = startTan ? p1 : p2; // Start point
  FaVec3 N   = startTan ? t ^ V12 : V12 ^ t; // Plane normal
  FaVec3 N12 = N ^ V12; // Vector pointing towards the center
  FaVec3 N1  = N ^ t;
  FaVec3 c   = findArcCenter(p0, N1, M12, N12);
  FaVec3 PM  = c + (p0-c).length()*(M12-c).normalize();

  return FFa3PArc(p1, PM, p2);
}


/*!
  Returns the length along the arc that will make the sagitta
  (distance from the chord to the arc) \a maxDeflection long.
  If \a maxDeflection is zero, the total arc length is returned.
*/

double FFa3PArc::getArcLength(double maxDeflection) const
{
  if (!this->isArc())
    return (P[2]-P[0]).length();

  double r = this->getRadius();
  if (maxDeflection > 0.0)
    return 2.0*r*acos(1.0-maxDeflection/r);

  FaVec3 c = this->getCenter();
  return (P[0]-c).angle(P[2]-c)*r;
}


FaVec3 FFa3PArc::getPointOnArc(double lengthFromStart) const
{
  if (!this->isArc())
    return P[0] + lengthFromStart*((P[2]-P[0]).normalize());

  FaMat34 cMx = this->getCtrlPointMatrix(0, this->getNormal());
  cMx[3] = this->getCenter();

  double r = this->getRadius();
  double a = lengthFromStart/r;
  return cMx * FaMat33::makeYrotation(a)[0]*r;
}


FaVec3 FFa3PArc::getTangent(double lengthFromStart) const
{
  if (!this->isArc())
    return (P[2]-P[0]).normalize();

  FaVec3 p = this->getPointOnArc(lengthFromStart);
  FaVec3 c = this->getCenter();
  FaVec3 n = this->getNormal();
  return (c-p) ^ n;
}
