// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_FACE_GENERATOR_H
#define FFL_FACE_GENERATOR_H

#include <vector>

class FFlVisFace;
class FFlVisEdge;
class FFlElementBase;
class FFlLinkHandler;
class FFlGeomUniqueTester;


class FFlFaceGenerator
{
public:
  FFlFaceGenerator(FFlLinkHandler* link);
  virtual ~FFlFaceGenerator();

protected:
  bool recreateSpecialEdges();
  void createGeometry(FFlElementBase* elm, FFlGeomUniqueTester& tester);
  void createBeamGeometry(FFlElementBase* elm, FFlGeomUniqueTester& tester);
  void createSpecialEdges(FFlElementBase* elm);

public:
  const std::vector<FFlVisFace*>& getFaces() const { return myVisFaces; }
  const std::vector<FFlVisEdge*>& getEccEd() const { return myBeamEccEdges; }

  virtual void dump() const;

protected:
  std::vector<FFlVisFace*> myVisFaces;     //!< All faces with results
  std::vector<FFlVisEdge*> myVisEdges;     //!< All edges with results
  std::vector<FFlVisEdge*> mySpecialEdges; //!< Spider element lines
  std::vector<FFlVisEdge*> myBeamEccEdges; //!< Beam eccentricity vectors
  std::vector<FFlVisEdge*> myBeamSysEdges; //!< Local beam system markers

private:
  FFlLinkHandler* myWorkLink;
};

#endif
