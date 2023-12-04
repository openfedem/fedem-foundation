// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_PYRAMID_GEOMETRY_H
#define FFA_PYRAMID_GEOMETRY_H

#include "FFaGeometryBase.H"


class FFaPyramidGeometry : public FFaGeometryBase
{
public:
  FFaPyramidGeometry() { mySide = myHeight = 0.0; }

  FFaPyramidGeometry(double side, double height)
  { mySide = side; myHeight = height; }

  virtual ~FFaPyramidGeometry() {}

  void setSideData(double side) { this->mySide = side; }
  double getSideData() const { return this->mySide; }

  void setHeightData(double height) { this->myHeight = height; }
  double getHeightData() const { return this->myHeight; }

  // Standard interface

  virtual FFaGeometryBase* getCopy() const;

  virtual bool isInside(const FaVec3& point, double tolerance) const;

  virtual bool isEqual(const FFaGeometryBase* geo) const;

protected:
  virtual std::ostream& writeStream(std::ostream& s) const;
  virtual std::istream& readStream(std::istream& s);

private:
  double mySide;
  double myHeight;
};

#endif
