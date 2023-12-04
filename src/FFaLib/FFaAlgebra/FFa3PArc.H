// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_3P_ARC_H
#define FFA_3P_ARC_H

#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"


class FFa3PArc
{
public:
  FFa3PArc(const FaVec3& p1, const FaVec3& p2, const FaVec3& p3)
  { P[0] = p1; P[1] = p2; P[2] = p3; }

  static FFa3PArc makeFromTangentP1P2(const FaVec3& t, const FaVec3& p1,
				      const FaVec3& p2, bool startTan = true);
  static FFa3PArc makeFromP1T1T2L(const FaVec3& p1, const FaVec3& t1,
				  const FaVec3& t2, double arcLength);

  static FaVec3 findCenter(const FaVec3& P1, FaVec3 P1C,
			   const FaVec3& P2, FaVec3 P2C);

  FaVec3 getCenter() const;
  FaVec3 getNormal() const;
  double getRadius() const;

  double getArcLength() const;
  FaVec3 getPointOnArc(double lengthFromStart) const;
  FaVec3 getTangent(double lengthFromStart) const;
  double getLengthWMaxDefl(double maxDeflection) const;

  FaMat34 getCtrlPointMatrix(int pointNumber, const FaVec3& positiveNormal,
			     bool normalIsSignOnly = true) const;

  bool isArc(double epsilon = 1.0e-10) const;
  bool isInside(const FaVec3& point) const;
  bool isOnCenterSide(const FaVec3& point) const;

  FaVec3 P[3];
};

#endif
