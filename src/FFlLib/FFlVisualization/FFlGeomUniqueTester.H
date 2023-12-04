// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_GEOM_UNIQUE_TESTER_H
#define FFL_GEOM_UNIQUE_TESTER_H

#include <vector>
#include <set>

#include "FFlLib/FFlVisualization/FFlVisFace.H"
#include "FFlLib/FFlVisualization/FFlVisEdge.H"
#include "FFlLib/FFlVertex.H"

typedef std::set<FFlVisEdge*,FFlVisEdge::FFlVisEdgeLess> FFlEdgeSet;
typedef std::set<FFlVisFace*,FFlVisFace::FFlVisFaceLess> FFlFaceSet;
typedef std::pair<FFlEdgeSet::const_iterator,bool> FFlEdgePtBoolPair;
typedef std::pair<FFlFaceSet::const_iterator,bool> FFlFacePtBoolPair;


class FFlGeomUniqueTester
{
public:
  FFlGeomUniqueTester(size_t vxCount = 25)
  {
    this->setEdgeHashSize(4*vxCount);
    this->setFaceHashSize(4*vxCount);
  }

  FFlEdgePtBoolPair insertEdge(FFlVisEdge* edge)
  {
    int lidx = edge->getFirstVxIdx();
    int uidx = edge->getSecondVxIdx();
    int hash = lidx*4 + lidx%2 + 2*(uidx%2);
    return myEdgeHash[hash].insert(edge);
  }

  FFlFacePtBoolPair insertFace(FFlVisFace* face)
  {
    VisEdgeRefVecCIter it = face->edgesBegin();
    int idx1 = it->getFirstVertex()->getRunningID(); ++it;
    if (face->getNumVertices() > 3) ++it;
    int idx2 = it->getFirstVertex()->getRunningID(); ++it;
    int idx3 = it->getFirstVertex()->getRunningID();
    int hash = 4*idx1 + (idx2%2) + 2*(idx3%2);
    return myFaceHash[hash].insert(face);
  }

private:
  void setEdgeHashSize(size_t size)
  {
    std::vector<FFlEdgeSet> eh(size);
    myEdgeHash.swap(eh);
  }

  void setFaceHashSize(size_t size)
  {
    std::vector<FFlFaceSet> fh(size);
    myFaceHash.swap(fh);
  }

  std::vector<FFlFaceSet> myFaceHash;
  std::vector<FFlEdgeSet> myEdgeHash;
};

#endif
