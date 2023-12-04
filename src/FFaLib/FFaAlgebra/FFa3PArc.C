// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaAlgebra/FFa3PArc.H"


bool FFa3PArc::isArc(double epsilon) const
{
  FaVec3 p1p2v = P[1]-P[0];
  FaVec3 p2p3v = P[2]-P[1];
  if (p1p2v.isZero(epsilon)) return false;
  if (p2p3v.isZero(epsilon)) return false;

  FaVec3 normal = p1p2v.normalize() ^ p2p3v.normalize();
  return normal.sqrLength() > epsilon*epsilon;
}


FaVec3 FFa3PArc::getCenter() const
{
  FaVec3 V12 = P[1]-P[0];
  FaVec3 V23 = P[2]-P[1];

  FaVec3 M12 = P[0] + 0.5*V12; // Midpoint p1 p2
  FaVec3 M23 = P[1] + 0.5*V23; // Midpoint p2 p3

  FaVec3 N   = V12 ^ V23; // Plane normal
  FaVec3 N12 = N   ^ V12; // Vectors pointing twowards center
  FaVec3 N23 = N   ^ V23; // Vectors pointing twowards center

  return FFa3PArc::findCenter(M12, N12, M23, N23);
}


/*!
  Finds the center of a circle based on two points on circle and
  vectors twowards centre at the two locations.

  This is done based on equations : P1 + a*P1C = C
                                    P2 + b*P2C = C

  Using x and y component equations as 4 equations with 4 unknowns.
*/

FaVec3 FFa3PArc::findCenter(const FaVec3& P1, FaVec3 P1C,
                            const FaVec3& P2, FaVec3 P2C)
{
  // Now, based on equations : P1 + a*P1C = C
  //                           P2 + b*P2C = C
  // Using x and y component equations as 4 equations with 4 unknowns
  // We get :

  // well, first find good component candidates.
  // We do not want to divide by zero and we need to use two directions.
  // That means X != Y and
  // P1C[X] != 0 and P2C[Y] != 0 and P1C[X]/P1C[Y] != P2C[X]/P2C[Y]
  // That is a result of the algebra needed to solve the equations.

  double maxSoundness = 0;
  double soundness;
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

  if (maxSoundness < 1e-10)
    return P1 + 1e10*P1C;

  double a = (P2[X]*P2C[Y] + P1[Y]*P2C[X] - P2[Y]*P2C[X] - P1[X]*P2C[Y]) /
    //       -----------------------------------------------------------
                           (P1C[X]*P2C[Y] - P1C[Y]*P2C[X]);

  return P1 + a*P1C;
}


FaVec3 FFa3PArc::getNormal() const
{
  FaVec3 p1p2v = P[1]-P[0];
  FaVec3 p2p3v = P[2]-P[1];
  FaVec3 normal = p1p2v ^ p2p3v;
  return normal.normalize();
}


double FFa3PArc::getRadius() const
{
  FaVec3 C = this->getCenter();
  FaVec3 CP1 = P[0] - C;
  return CP1.length();
}


bool FFa3PArc::isInside(const FaVec3& point) const
{
  FaVec3 C = this->getCenter();
  FaVec3 CP = point - C;
  FaVec3 CP1 = P[0] - C;
  return CP.length() <= CP1.length();
}


bool FFa3PArc::isOnCenterSide(const FaVec3& point) const
{
  return this->isInside(point);
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
  FaVec3 pm;
  FaVec3 p2;
  switch (t1.isParallell(t2))
    {
    case 1: // line
      p2 = p1 + arcLength*t1;
      pm = p1 + 0.5*arcLength*t1;
      break;
    case -1: // Complete circle
      p2 = p1;
      pm = p1;
      break;
    case 0:
      {
        double ang = t1.angle(t2);
        double r = arcLength/ang;
        FaVec3 n = t1 ^ t2;
        n.normalize();

        FaVec3 et1 = t1; et1.normalize();
        FaVec3 et2 = t2; et2.normalize();
        FaVec3 eP1C = n  ^ et1;
        FaVec3 eP2C = n ^ et2;
        FaVec3 c = p1 + r * eP1C;
        p2 = c - r*eP2C;

        FaVec3 M12 = p1 + 0.5*(p2-p1);
        pm = c + r*(M12-c).normalize();
      }
      break;
    }

  return FFa3PArc(p1, pm,  p2);
}


FFa3PArc FFa3PArc::makeFromTangentP1P2(const FaVec3& t, const FaVec3& p1,
				       const FaVec3& p2, bool isStartTangent)
{
  FaVec3 V12 = p2-p1;
  FaVec3 M12 = p1 + 0.5*V12; // Midpoint p1 p2
  FaVec3 PM;

  // Check if it is a line
  switch (t.isParallell(V12))
    {
    case 1: // Line P1 M12 P2
      PM = M12;
      break;
    case -1: // Degenerated line p1 -inf+, p2
      PM = p1-0.5*V12;
      break;
    case 0:
      { // Arc
	FaVec3 N;
	if (isStartTangent)
	  N = t ^ V12; // Plane normal
	else
	  N = V12 ^ t; // Plane normal

	FaVec3 N12 = N ^ V12; // Vector pointing twowards center
	FaVec3 N1  = N ^ t;
	FaVec3 tPoint;
	if (isStartTangent)
	  tPoint = p1;
	else
	  tPoint = p2;

	FaVec3 c = FFa3PArc::findCenter(tPoint, N1, M12, N12);
	PM = c + (tPoint-c).length()*(M12-c).normalize();
      }
      break;
    }

  return FFa3PArc(p1, PM, p2);
}


/*!
  Returns the length of this arc along the arc.
*/

double FFa3PArc::getArcLength() const
{
  if (!this->isArc())
    return (P[2]-P[0]).length();

  double r = this->getRadius();
  FaVec3 c = this->getCenter();
  FaVec3 cP1 = P[0]-c;
  FaVec3 cP3 = P[2]-c;
  return cP1.angle(cP3)*r;
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
  if (!isArc())
    return (P[2]-P[0]).normalize();

  FaVec3 p = this->getPointOnArc(lengthFromStart);
  FaVec3 c = this->getCenter();
  FaVec3 n = this->getNormal();
  return (c-p) ^ n;
}


/*!
  Returns the length along the arc that will make the sagitta
  (distance from the chord to the arc) \sa maxDeflection long.
*/

double FFa3PArc::getLengthWMaxDefl(double maxDeflection) const
{
  if (!this->isArc())
    return (P[2]-P[0]).length();

  double r = this->getRadius();
  return (r+r)*acos(1-maxDeflection/r);
}

