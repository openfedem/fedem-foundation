// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVertex.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


FFlVertex::FFlVertex(double x, double y, double z) : FaVec3(x,y,z)
{
  myRunningID = myRefCount = 0;
  myNode = NULL;
}


FFlVertex::FFlVertex(const FaVec3& v) : FaVec3(v)
{
  myRunningID = myRefCount = 0;
  myNode = NULL;
}


FFlVertex::FFlVertex(const FFlVertex& v) : FaVec3(v)
{
  // create a new object based on the original - no multi-instancing
  myRunningID = myRefCount = 0;
  myNode = NULL;
}


short int FFlVertex::unRef()
{
  if (--myRefCount > 0)
    return myRefCount;

  delete this;
  return 0;
}

#ifdef FF_NAMESPACE
} // namespace
#endif
