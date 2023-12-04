// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlFEElementTopSpec.H"
#include "FFlrLib/FFlrFEResult.H"

#ifdef FT_USE_MEMPOOL
FFaMemPool FFlFEElmResult::ourMemPool(sizeof(FFlFEElmResult));
FFaMemPool FFlFENodeResult::ourMemPool(sizeof(FFlFENodeResult));
#endif


FFlrFELinkResult::~FFlrFELinkResult()
{
  if (transformReader)
    transformReader->unref();

  for (FFaOperation<FaVec3>* op : deformationOps)
    if (op) op->unref();

  for (FFaOperation<double>* op : scalarOps)
    if (op) op->unref();
}


/////////////////////////////////////////////////////////////////////////
//
//  Get result implementations in the FFl-objects.
//  Implemented here to keep FFlLib clean.
//
/////////////////////////////////////////////////////////////////////////

FFlrFELinkResult* FFlLinkHandler::getResults()
{
  if (!myResults)
    myResults = new FFlrFELinkResult;

  return static_cast<FFlrFELinkResult*>(myResults);
}

FFlFENodeResult* FFlNode::getResults()
{
  if (!myResults)
    myResults = new FFlFENodeResult;

  return static_cast<FFlFENodeResult*>(myResults);
}

FFlFEElmResult* FFlElementBase::getResults()
{
  if (!myResults)
    myResults = new FFlFEElmResult(this->getFEElementTopSpec()->getNodeCount(),
                                   this->getFEElementTopSpec()->getExpandedNodeCount());

  return static_cast<FFlFEElmResult*>(myResults);
}
