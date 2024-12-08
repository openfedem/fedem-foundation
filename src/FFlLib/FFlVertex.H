// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VERTEX_H
#define FFL_VERTEX_H

#include "FFaLib/FFaAlgebra/FFaVec3.H"
#ifdef FT_USE_MEMPOOL
#include "FFaLib/FFaPatterns/FFaMemPool.H"
#endif

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif

class FFlNode;


class FFlVertex : public FaVec3
{
public:
  FFlVertex(double x = 0.0, double y = 0.0, double z = 0.0);
  FFlVertex(const FaVec3& v);
  FFlVertex(const FFlVertex& v);
  virtual ~FFlVertex() {}

  // Delete the assignment operator
  FFlVertex& operator=(const FFlVertex&) = delete;

  FFlNode* getNode() const { return myNode; }
  void setNode(FFlNode* node) { myNode = node; }

  // reference counting
  short int ref() { return ++myRefCount; }
  short int unRef();

  // container positioning
  int getRunningID() const   { return myRunningID; }
  void setRunningID(int rID) { myRunningID = rID; }

protected:
  short int myRefCount;
  int       myRunningID;
  FFlNode*  myNode;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
