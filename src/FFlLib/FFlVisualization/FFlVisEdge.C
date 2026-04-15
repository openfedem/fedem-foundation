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
  for (int i = 0; i < 2; i++)
    if ((myVertex[i] = edg.getVertex(i)))
      myVertex[i]->ref();

  myRefCount = edg.myRefCount;
  myRenderData = NULL;
}


FFlVisEdge& FFlVisEdge::operator=(const FFlVisEdge& edg)
{
  if (this != &edg)
  {
    this->releaseVertices();

    for (int i = 0; i < 2; i++)
      if ((myVertex[i] = edg.getVertex(i)))
        myVertex[i]->ref();
  }

  return *this;
}


bool FFlVisEdge::setVertices(FFlVertex* n1, FFlVertex* n2, bool constructing)
{
  if (!constructing)
    this->releaseVertices();

  // Check if the edge direction is swapped
  int i1 = n1 && n2 && n1->getRunningID() > n2->getRunningID() ? 1 : 0;

  myVertex[i1] = n1;
  myVertex[1-i1] = n2;

  for (int i = 0; i < 2; i++)
    if (myVertex[i])
      myVertex[i]->ref();

  return i1 == 0; // if swapped then negative direction
}


/*!
  Get running vertex indices for edge.
*/

void FFlVisEdge::getEdgeVertices(std::vector<int>& vertexRefs) const
{
  vertexRefs.reserve(2);
  for (int idx = 0; idx < 2; idx++)
    if (myVertex[idx])
      vertexRefs.push_back(myVertex[idx]->getRunningID());
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
  for (int idx = 0; idx < 2; idx++)
    if (myVertex[idx])
      *(vertexIdxArrayPtr++) = myVertex[idx]->getRunningID();
}


int FFlVisEdge::getVertexIdx(int idx) const
{
  if (idx < 0 || idx > 1)
    return -99;

  return myVertex[idx] ? myVertex[idx]->getRunningID() : -1-idx;
}

int FFlVisEdge::getFirstVxIdx() const
{
  return myVertex[0] ? myVertex[0]->getRunningID() : -1;
}

int FFlVisEdge::getSecondVxIdx() const
{
  return myVertex[1] ? myVertex[1]->getRunningID() : -2;
}


void FFlVisEdge::releaseVertices()
{
  for (int idx = 0; idx < 2; idx++)
    if (myVertex[idx])
      myVertex[idx]->unRef();

  myVertex[0] = myVertex[1] = NULL;
}


/*!
  Returns the vector from the first vertex to the second.
*/

FaVec3 FFlVisEdge::getVector() const
{
  if (myVertex[0] && myVertex[1])
    return *myVertex[1] - *myVertex[0];
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


int FFlVisEdge::ref()
{
  if (myRefCount < 254)
    myRefCount++;
  else
    std::cout <<"  ** FFlVisEdge::ref(): More than 255 references to one edge,"
              <<" reference counting will be incorrect."<< std::endl;

  return myRefCount;
}


int FFlVisEdge::unRef()
{
  if (myRefCount > 0)
    myRefCount--;

  return myRefCount;
}


bool FFlVisEdge::FFlVisEdgeLess::operator()(const FFlVisEdge* a,
                                            const FFlVisEdge* b) const
{
  int aID = a->getFirstVxIdx();
  int bID = b->getFirstVxIdx();

  if (aID == bID)
    return a->getSecondVxIdx() < b->getSecondVxIdx();
  else
    return aID < bID;
}

bool FFlVisEdge::FFlVisEdgeEqual::operator()(const FFlVisEdge* a,
                                             const FFlVisEdge* b) const
{
  return (a->getFirstVxIdx()  == b->getFirstVxIdx() &&
          a->getSecondVxIdx() == b->getSecondVxIdx());
}


////////////////////////////////////////////////////////////////////////

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


int FFlVisEdgeRef::getFirstVxIdx() const
{
  FFlVertex* vtx = this->getFirstVertex();
  return vtx ? vtx->getRunningID() : -1;
}

int FFlVisEdgeRef::getSecondVxIdx() const
{
  FFlVertex* vtx = this->getSecondVertex();
  return vtx ? vtx->getRunningID() : -2;
}


std::ostream& operator<<(std::ostream& os, const FFlVisEdgeRef& a)
{
  if (!a.myVisEdge)
    return os <<"(NULL)";
  else
    return os <<"["<< a.getFirstVxIdx()
              <<"]->["<< a.getSecondVxIdx() <<"]";
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
