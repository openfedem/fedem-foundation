// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVisualization/FFlVisEdge.H"
#include "FFlLib/FFlVertex.H"


#ifdef FT_USE_MEMPOOL
FFaMemPool FFlVisEdge::ourMemPool(sizeof(FFlVisEdge));
FFaMemPool FFlVisEdgeRenderData::ourMemPool(sizeof(FFlVisEdgeRenderData));
#endif


FFlVisEdge::FFlVisEdge(FFlVertex* n1, FFlVertex* n2)
{
  this->setVertices(n1,n2,true);

  myRefCount = 0;
  myRenderData = NULL;
}


FFlVisEdge::FFlVisEdge(const FFlVisEdge& edg)
{
  myFirstVertex = edg.getFirstVertex();
  if (myFirstVertex)
    myFirstVertex->ref();

  mySecVertex = edg.getSecondVertex();
  if (mySecVertex)
    mySecVertex->ref();

  myRefCount = edg.myRefCount;
  myRenderData = NULL;
}


FFlVisEdge& FFlVisEdge::operator=(const FFlVisEdge& edg)
{
  if (this != &edg)
  {
    this->releaseVertices();

    myFirstVertex = edg.getFirstVertex();
    if (myFirstVertex)
      myFirstVertex->ref();

    mySecVertex = edg.getSecondVertex();
    if (mySecVertex)
      mySecVertex->ref();
  }

  return *this;
}


bool FFlVisEdge::setVertices(FFlVertex* n1, FFlVertex* n2, bool constructing)
{
  if (!constructing)
    this->releaseVertices();

  bool doSwap = n1 && n2 && n1->getRunningID() > n2->getRunningID();
  if (doSwap)
  {
    myFirstVertex = n2;
    mySecVertex = n1;
  }
  else
  {
    myFirstVertex = n1;
    mySecVertex = n2;
  }

  if (myFirstVertex)
    myFirstVertex->ref();
  if (mySecVertex)
    mySecVertex->ref();

  return !doSwap; // If swapped then negative direction
}


/*!
  Get running vertex indices for edge.
*/

void FFlVisEdge::getEdgeVertices(std::vector<int>& vertexRefs) const
{
  vertexRefs.reserve(2);

  if (myFirstVertex)
    vertexRefs.push_back(myFirstVertex->getRunningID());

  if (mySecVertex)
    vertexRefs.push_back(mySecVertex->getRunningID());
}

/*!
  Get running vertex indices for edge by storing
  them at the location pointed to by vertexIdxArrayPtr.

  It is assumed to point to pre allocated storage of sufficient size.

  On return the vertexIdxArrayPtr
  points to the first array location after the edge.
*/

void FFlVisEdge::getEdgeVertices(int*& vertexIdxArrayPtr) const
{
  if (myFirstVertex)
    *(vertexIdxArrayPtr++) = myFirstVertex->getRunningID();

  if (mySecVertex)
    *(vertexIdxArrayPtr++) = mySecVertex->getRunningID();
}


int FFlVisEdge::getFirstVxIdx() const
{
  return myFirstVertex->getRunningID();
}

int FFlVisEdge::getSecondVxIdx() const
{
  return mySecVertex->getRunningID();
}


void FFlVisEdge::releaseVertices()
{
  if (myFirstVertex)
    myFirstVertex->unRef();
  if (mySecVertex)
    mySecVertex->unRef();

  myFirstVertex = mySecVertex = NULL;
}


/*!
  Returns the vector from the first vertex to the second.
*/

FaVec3 FFlVisEdge::getVector() const
{
  if (myFirstVertex && mySecVertex)
    return *mySecVertex - *myFirstVertex;
  else
    return FaVec3();
}


FFlVisEdgeRenderData* FFlVisEdge::getRenderData()
{
  if (!myRenderData)
    myRenderData = new FFlVisEdgeRenderData;
  return myRenderData;
}

void FFlVisEdge::deleteRenderData()
{
  delete myRenderData;
  myRenderData = NULL;
}


bool FFlVisEdge::FFlVisEdgeLess::operator()(const FFlVisEdge* first, const FFlVisEdge* sec) const
{
  register int firstID = first->getFirstVertex()->getRunningID();
  register int secID   = sec->getFirstVertex()->getRunningID();

  if (firstID == secID)
    return (first->getSecondVertex()->getRunningID() < sec->getSecondVertex()->getRunningID());
  else
    return (firstID < secID);
}

bool FFlVisEdge::FFlVisEdgeEqual::operator()(const FFlVisEdge* first, const FFlVisEdge* sec) const
{
  return (first->getFirstVertex()->getRunningID()  == sec->getFirstVertex()->getRunningID() &&
          first->getSecondVertex()->getRunningID() == sec->getSecondVertex()->getRunningID());
}


////////////////////////////////////////////////////////////////////////

FFlVisEdgeRef::FFlVisEdgeRef(FFlVisEdge* edge)
{
  iAmPositive = true;
  myVisEdge = edge;
  if (myVisEdge)
    myVisEdge->ref();
}


FFlVisEdgeRef::FFlVisEdgeRef(const FFlVisEdgeRef& ref)
{
  iAmPositive = ref.iAmPositive;
  myVisEdge = ref.myVisEdge;
  if (myVisEdge)
    myVisEdge->ref();
}


FFlVisEdgeRef& FFlVisEdgeRef::operator=(const FFlVisEdge* e)
{
  if (myVisEdge != e)
  {
    if (myVisEdge)
      myVisEdge->unRef();
    myVisEdge = const_cast<FFlVisEdge*>(e);
    if (myVisEdge)
      myVisEdge->ref();
  }

  return *this;
}


FFlVisEdgeRef& FFlVisEdgeRef::operator=(const FFlVisEdgeRef& ref)
{
  if (this != &ref)
  {
    iAmPositive = ref.iAmPositive;
    this->operator=(ref.myVisEdge);
  }

  return *this;
}


FFlVertex* FFlVisEdgeRef::getFirstVertex() const
{
  if (!myVisEdge)
    return NULL;

  if (iAmPositive)
    return myVisEdge->getFirstVertex();
  else
    return myVisEdge->getSecondVertex();
}

FFlVertex* FFlVisEdgeRef::getSecondVertex() const
{
  if (!myVisEdge)
    return NULL;

  if (iAmPositive)
    return myVisEdge->getSecondVertex();
  else
    return myVisEdge->getFirstVertex();
}


bool operator<(const FFlVisEdgeRef& a, const FFlVisEdgeRef& b)
{
  return FFlVisEdge::FFlVisEdgeLess()(a.getEdge(),b.getEdge());
}

bool operator==(const FFlVisEdgeRef& a, const FFlVisEdgeRef& b)
{
  return FFlVisEdge::FFlVisEdgeEqual()(a.getEdge(),b.getEdge());
}

bool operator>(const FFlVisEdgeRef& a, const FFlVisEdgeRef& b)
{
  return (!(a < b) && !(a == b));
}

bool operator<=(const FFlVisEdgeRef& a, const FFlVisEdgeRef& b)
{
  return (a < b || a == b);
}

bool operator>=(const FFlVisEdgeRef& a, const FFlVisEdgeRef& b)
{
  return (a > b || a == b);
}
