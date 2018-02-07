// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////
/*!
  \file FFa3PArc.H
  \brief Circular arcs in 3D space.
*/

#ifndef FFA_3P_ARC_H
#define FFA_3P_ARC_H

#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include <array>

class FaMat34;


/*!
  \brief Class representing a circular arc in 3D space.
  \details Mainly used to model cam joint and contact element master curves.
*/

class FFa3PArc
{
public:
  //! \brief The constructor initializes the three points defining the arc.
  FFa3PArc(const FaVec3& p1, const FaVec3& p2, const FaVec3& p3)
  { P[0] = p1; P[1] = p2; P[2] = p3; }

  //! \brief Creates an arc from two end points and a tangent vector.
  static FFa3PArc makeFromTangentP1P2(const FaVec3& t, const FaVec3& p1,
				      const FaVec3& p2, bool startTan = true);
  //! \brief Creates an arc from an end point and two tangent vectors.
  static FFa3PArc makeFromP1T1T2L(const FaVec3& p1, const FaVec3& t1,
				  const FaVec3& t2, double arcLength);

  //! \brief Returns the centre point of the arc.
  FaVec3 getCenter() const;
  //! \brief Returns the normal vector of the arc plane.
  FaVec3 getNormal() const;
  //! \brief Returns the radius of the arc.
  double getRadius() const;

  //! \brief Returns the length of the arc.
  double getArcLength(double maxDeflection = 0.0) const;
  //! \brief Returns the point a given distance from the start point.
  FaVec3 getPointOnArc(double lengthFromStart) const;
  //! \brief Returns the tangent vector at a given distance from the start.
  FaVec3 getTangent(double lengthFromStart) const;
  //! \brief Returns the secant vector of the arc.
  FaVec3 getSecant() const { return P.back() - P.front(); }

  //! \brief Returns the start point.
  const FaVec3& front() const { return P.front(); }
  //! \brief Returns the end point.
  const FaVec3& back() const { return P.back(); }

  //! \brief Returns the full position matrix for one of the arc points.
  FaMat34 getCtrlPointMatrix(int pointNumber, const FaVec3& positiveNormal,
			     bool normalIsSignOnly = true) const;

  //! \brief Returns \e false if this arc is a straight line.
  bool isArc(double epsilon = 1.0e-10) const;
  //! \brief Returns \e true if given point is inside the arc.
  bool isInside(const FaVec3& point) const;

private:
  std::array<FaVec3,3> P; //!< The three points defining this arc
};

#endif
