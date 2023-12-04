// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_SPHERE_GEOMETRY_H
#define FFA_SPHERE_GEOMETRY_H

#include "FFaGeometryBase.H"


class FFaSphereGeometry : public FFaGeometryBase
{
public:
  FFaSphereGeometry() { myRadius = myAngle = 0.0; }

  FFaSphereGeometry(double radius, double angle)
  { myRadius = radius; myAngle = angle; }

  virtual ~FFaSphereGeometry() {}

  void setRadius(double radius) { this->myRadius = radius; }
  double getRadius() const { return this->myRadius; }

  void setAngleData(double angle) { this->myAngle = angle; }
  double getAngleData() const { return this->myAngle; }

  // Standard interface

  virtual FFaGeometryBase* getCopy() const;

  virtual bool isInside(const FaVec3& point, double tolerance) const;

  virtual bool isEqual(const FFaGeometryBase* geo) const;

protected:
  virtual std::ostream& writeStream(std::ostream& s) const;
  virtual std::istream& readStream(std::istream& s);

private:
  double myRadius;
  double myAngle;
};

#endif
