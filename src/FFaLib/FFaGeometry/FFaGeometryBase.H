// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_GEOMETRY_BASE_H
#define FFA_GEOMETRY_BASE_H

#include "FFaLib/FFaAlgebra/FFaMat34.H"


namespace FFa
{
  const double geomEpsilon = 1.0e-5;
};

class FFaGeometryBase
{
protected:
  FFaGeometryBase() { myAddExclude = true; }

  virtual std::istream& readStream(std::istream& s) { return s; }
  virtual std::ostream& writeStream(std::ostream& s) const { return s; }

public:
  virtual ~FFaGeometryBase() {}

  const FaMat34& getTransMatrix() const { return myPosition; }
  void setTransMatrix(const FaMat34& transMat) { myPosition = transMat; }

  bool getAddExclude() const { return myAddExclude; }
  void setAddExclude(bool addExclude) { myAddExclude = addExclude; }

  virtual FFaGeometryBase* getCopy() const = 0;
  virtual bool isInside(const FaVec3& point, double tolerance) const = 0;
  virtual bool isEqual(const FFaGeometryBase* geo) const = 0;

  // Global stream operators
  friend std::ostream& operator<<(std::ostream& s, const FFaGeometryBase& geo)
  { return geo.writeStream(s); }
  friend std::istream& operator>>(std::istream& s, FFaGeometryBase& geo)
  { return geo.readStream(s); }

protected:
  FaMat34 myPosition;
  bool    myAddExclude;
};

#endif
