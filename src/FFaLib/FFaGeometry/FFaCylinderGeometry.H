// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_CYLINDER_GEOMETRY_H
#define FFA_CYLINDER_GEOMETRY_H

#include <vector>

#include "FFaGeometryBase.H"


class FFaCylinderGeometry : public FFaGeometryBase
{
public:
  FFaCylinderGeometry();

  FFaCylinderGeometry(const std::vector<FaVec3>& points, bool arcOnly = false)
  { this->define(points,arcOnly); }

  virtual ~FFaCylinderGeometry() {}

  void define(const std::vector<FaVec3>& points, bool sector);

  void setRadius(double radius) { this->myRadius = radius; }
  double getRadius() const { return this->myRadius; }

  void setZData(double zStart, double zEnd);
  std::pair<double,double> getZData() const;

  void setAngleData(double angleStart, double angleEnd);
  std::pair<double,double> getAngleData() const;

  // Standard interface

  virtual FFaGeometryBase* getCopy() const;
  virtual bool isInside(const FaVec3& point, double tolerance) const;
  virtual bool isEqual(const FFaGeometryBase* geo) const;

protected:
  virtual std::ostream& writeStream(std::ostream& s) const;
  virtual std::istream& readStream(std::istream& s);

private:
  double myRadius;
  double myZStart;
  double myZEnd;
  double myAngleStart;
  double myAngleEnd;
};

#endif
