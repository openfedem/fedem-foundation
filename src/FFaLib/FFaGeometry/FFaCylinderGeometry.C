// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaAlgebra/FFa3PArc.H"
#include "FFaLib/FFaAlgebra/FFaMath.H"
#include "FFaLib/FFaGeometry/FFaCylinderGeometry.H"


FFaCylinderGeometry::FFaCylinderGeometry()
{
  myRadius     = 1.0;
  myZStart     =-HUGE_VAL;
  myZEnd       = HUGE_VAL;
  myAngleStart = 0.0;
  myAngleEnd   = 2.0*M_PI;
}


/*!
  Define this cylinder by supplying a 3-point circle, and start and end points.
  If only 3 points are given, a circle is created.
  If 4 points are given the cylinder goes from the circle to the 4th point.
  If five points are given, the cylinder is defined from 4th to 5th point.
  The \a sector argument decides whether to use a sector cylinder,
  or a complete cylinder.
  The sector is defined by the angle from the 1st to the 3rd point.
*/

void FFaCylinderGeometry::define(const std::vector<FaVec3>& points, bool sector)
{
  if (points.size() < 3) return;

  FFa3PArc circle(points[0], points[1], points[2]);
  FaVec3 center = circle.getCenter();
  double radius = circle.getRadius();
  double zStart = 0.0, zEnd = 0.0;

  // Make the CS of the cylinder
  FaVec3 ex = (points[0] - center).normalize();
  FaVec3 ez = circle.getNormal();
  FaVec3 ey = (ez ^ ex).normalize();
  FaMat34 transMat(ex, ey, ez, center);

  if (points.size() == 4)
  {
    // Cylinder is defined from circle to "start point"
    zEnd = (transMat.inverse()*points[3])[2];
  }
  else if (points.size() == 5)
  {
    // Cylinder defined from start to end
    zStart = (transMat.inverse()*points[3])[2];
    zEnd   = (transMat.inverse()*points[4])[2];
  }

  this->setAngleData(0.0, sector ? ex.angle(points[2]-center) : 2.0*M_PI);
  this->setRadius(radius);
  this->setZData(zStart,zEnd);
  this->setTransMatrix(transMat);
}


FFaGeometryBase* FFaCylinderGeometry::getCopy() const
{
  return new FFaCylinderGeometry(*this);
}


void FFaCylinderGeometry::setZData(double zStart, double zEnd)
{
  this->myZStart = zStart < zEnd ? zStart : zEnd;
  this->myZEnd   = zStart < zEnd ? zEnd   : zStart;
}

std::pair<double,double> FFaCylinderGeometry::getZData() const
{
  return std::make_pair(this->myZStart,this->myZEnd);
}


void FFaCylinderGeometry::setAngleData(double angleStart, double angleEnd)
{
  this->myAngleStart = angleStart < angleEnd ? angleStart : angleEnd;
  this->myAngleEnd   = angleStart < angleEnd ? angleEnd   : angleStart;
}


std::pair<double,double> FFaCylinderGeometry::getAngleData() const
{
  return std::make_pair(this->myAngleStart,this->myAngleEnd);
}


/*!
  Returns true if a point lies on the boundary surface (or inside) a cylinder.
*/

bool FFaCylinderGeometry::isInside(const FaVec3& point, double tolerance) const
{
  // Negate the tolerance if we actually are doing an is-outside check
  if (!myAddExclude) tolerance = -tolerance;

  FaVec3 pTrans = myPosition.inverse() * point;
  double ax = pTrans[0];
  double ay = pTrans[1];
  double R  = myRadius + tolerance;
  if (ax*ax + ay*ay > R*R) return false;

  double Z0 = myZStart - tolerance;
  double Z1 = myZEnd + tolerance;
  if (pTrans[2] < Z0 || pTrans[2] > Z1) return false;

  double a1 = myAngleStart;
  double a2 = myAngleEnd;
  if (a2 - a1 >= 2.0*M_PI) return true;

  FaVec3 xVec(1.0, 0.0, 0.0);
  FaVec3 pointVec(ax, ay, 0.0);
  double angle = xVec.angle(pointVec);
  if (ay < 0.0) angle = 2.0*M_PI - angle; // angle is negative if ay<0

  a1 -= tolerance/myRadius;
  a2 += tolerance/myRadius;
  return angle >= a1 && angle <= a2;
}


std::ostream& FFaCylinderGeometry::writeStream(std::ostream& s) const
{
  s <<"CYLINDER "<< this->myAddExclude << '\n';
  s << this->myRadius << ' ' << this->myZStart << ' ' << this->myZEnd << ' '
    << this->myAngleStart << ' ' << this->myAngleEnd;
  s << this->myPosition;
  return s << std::endl;
}


std::istream& FFaCylinderGeometry::readStream(std::istream& s)
{
  s >> this->myAddExclude;
  s >> this->myRadius >> this->myZStart >> this->myZEnd
    >> this->myAngleStart >> this->myAngleEnd;
  s >> this->myPosition;
  return s;
}


bool FFaCylinderGeometry::isEqual(const FFaGeometryBase* geo) const
{
  const FFaCylinderGeometry* cg = dynamic_cast<const FFaCylinderGeometry*>(geo);
  if (!cg) return false;

  return (myRadius     == cg->myRadius     &&
          myZStart     == cg->myZStart     &&
          myZEnd       == cg->myZEnd       &&
          myAngleStart == cg->myAngleStart &&
          myAngleEnd   == cg->myAngleEnd   &&
          myPosition   == cg->myPosition   &&
          myAddExclude == cg->myAddExclude);
}
