// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_LINE_GEOMETRY_H
#define FFA_LINE_GEOMETRY_H

#include "FFaGeometryBase.H"


class FFaLineGeometry : public FFaGeometryBase
{
public:
  FFaLineGeometry() : myXaxis(myPosition[VX]), myOrigin(myPosition[VW]) {}
  FFaLineGeometry(const FaVec3& P0, const FaVec3& P1);
  virtual ~FFaLineGeometry() {}

  virtual FFaGeometryBase* getCopy() const;
  virtual bool isInside(const FaVec3& point, double tolerance) const;
  virtual bool isEqual(const FFaGeometryBase* geo) const;

protected:
  virtual std::ostream& writeStream(std::ostream& s) const;
  virtual std::istream& readStream(std::istream& s);

private:
  FaVec3& myXaxis;
  FaVec3& myOrigin;
};

#endif
