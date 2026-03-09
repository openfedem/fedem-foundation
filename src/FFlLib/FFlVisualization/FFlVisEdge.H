// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VIS_EDGE_H
#define FFL_VIS_EDGE_H

#include <vector>
#include <utility>
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#ifdef FT_USE_MEMPOOL
#include "FFaLib/FFaPatterns/FFaMemPool.H"
#endif

class FFlVertex;
class FFlVisFace;
using FFlFaceRef = std::pair<FFlVisFace*,FaVec3>;


/*!
  Used to store status concerning geometry reduction from FFlGroupPartCreator.
  \sa FFlGroupPartCreator
*/

struct FFlVisEdgeRenderData
{
  enum RenderStatus {INTERNAL, SURFACE, OUTLINE};

  unsigned short linePattern = 0xffff;
  bool           simplified = false;
  char           edgeStatus = INTERNAL; // RenderStatus

  std::vector<FFlFaceRef> faceReferences;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};


class FFlVisEdge
{
public:
  FFlVisEdge(FFlVertex* n1 = NULL, FFlVertex* n2 = NULL);
  ~FFlVisEdge() { this->releaseVertices(); this->deleteRenderData(); }

  // Copy and assignment :

  FFlVisEdge(const FFlVisEdge& edg);
  FFlVisEdge& operator=(const FFlVisEdge& edg);

  // Vertex access :

  bool setVertices(FFlVertex* n1, FFlVertex* n2, bool constructing = false);
  FFlVertex* getFirstVertex() const { return myVertex[0]; }
  FFlVertex* getSecondVertex() const { return myVertex[1]; }
  FFlVertex* getVertex(int idx) const { return myVertex[idx]; }

  // Get running vertex indices for edge

  void getEdgeVertices(std::vector<int>& vertexRefs) const;
  void getEdgeVertices(int*& vertexIdxArrayPtr) const;
  int  getFirstVxIdx() const;
  int  getSecondVxIdx() const;

  // Get vector along edge

  FaVec3 getVector() const;

  // Do unreffing of vertexes

  void releaseVertices();

  // Ref-counting from faces : myRefCount = nFaces reffing this edge:

  int ref()           { if(myRefCount < 254) myRefCount++; return myRefCount; }
  int unRef()         { if(myRefCount > 0)   myRefCount--; return myRefCount; }
  int getRefs() const { return myRefCount; }

  // Render data used by groupPartGenerator to store status etc.

  FFlVisEdgeRenderData* getRenderData();
  void deleteRenderData();

protected:
  unsigned char         myRefCount;
  FFlVertex*            myVertex[2];
  FFlVisEdgeRenderData* myRenderData;

public:
  struct FFlVisEdgeLess
  {
    bool operator()(const FFlVisEdge* a, const FFlVisEdge* b) const;
  };

  struct FFlVisEdgeEqual
  {
    bool operator()(const FFlVisEdge* a, const FFlVisEdge* b) const;
  };

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};


class FFlVisEdgeRef
{
public:
  FFlVisEdgeRef(bool pos = true) : iAmPositive(pos), myVisEdge(NULL) {}
  FFlVisEdgeRef(const FFlVisEdgeRef& ref);

  ~FFlVisEdgeRef() { if (myVisEdge) myVisEdge->unRef(); }

  FFlVisEdge* operator->() const { return myVisEdge; }
  FFlVisEdge* getEdge() const { return myVisEdge; }
  FFlVisEdgeRef& operator=(const FFlVisEdgeRef& ref);
  FFlVisEdgeRef& operator=(const FFlVisEdge& e) { return this->operator=(&e); }
  FFlVisEdgeRef& operator=(const FFlVisEdge* e);

  bool isPosDir() const { return iAmPositive; }
  void setPosDir(bool pos) { iAmPositive = pos;}

  // Returns correct oriented edge vertices.

  FFlVertex* getFirstVertex()  const;
  FFlVertex* getSecondVertex() const;

protected:
  bool        iAmPositive; // true if the user sees the edge in the same direction as the edge itself
  FFlVisEdge* myVisEdge;
};


bool operator< (const FFlVisEdgeRef& a, const FFlVisEdgeRef& b);
bool operator==(const FFlVisEdgeRef& a, const FFlVisEdgeRef& b);
bool operator> (const FFlVisEdgeRef& a, const FFlVisEdgeRef& b);
bool operator<=(const FFlVisEdgeRef& a, const FFlVisEdgeRef& b);
bool operator>=(const FFlVisEdgeRef& a, const FFlVisEdgeRef& b);

#endif
