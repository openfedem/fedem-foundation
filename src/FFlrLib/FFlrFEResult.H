// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFLR_FE_RESULT_H
#define FFLR_FE_RESULT_H

#include "FFlLib/FFlFEResultBase.H"
#include "FFaLib/FFaOperation/FFaOperation.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#ifdef FT_USE_MEMPOOL
#include "FFaLib/FFaPatterns/FFaMemPool.H"
#endif
#include <vector>


/*!
  \file FFlrFEResult.H
  \brief Classes to store results and FFr-references.

  \details
  Used by Elements and Nodes to store results and refs. into the FFrExtractor
  data structures. Results are stored only for one time step, but the containers
  can keep results of different mapping, and type at the same time.

  The containers are to be used as temporary storage during reading,
  transforming and finally generating the visual representation of the results.
*/

/////////////////////////////////////////////////////////////////////////
//
// FFlrFELinkResult
//
/////////////////////////////////////////////////////////////////////////

class FFlrFELinkResult : public FFlFEResultBase
{
public:
  FFlrFELinkResult() : transformReader(NULL) {}
  virtual ~FFlrFELinkResult();

  // Operation tree root. Used to do actual evaluation
  FFaOperation<FaMat34>*             transformReader;
  std::vector<FFaOperation<FaVec3>*> deformationOps;
  std::vector<FFaOperation<double>*> scalarOps;
  std::vector<int>                   elmStart;
};


/////////////////////////////////////////////////////////////////////////
//
// FFlFEElmResult
//
/////////////////////////////////////////////////////////////////////////

class FFlFEElmResult : public FFlFEResultBase
{
public:
  FFlFEElmResult(char nodeCount, char evalPointCount)
  {
    elmResSetMrg = myAverager = NULL;
    ENRSMrgs = EPRSMrgs = NULL;

    myElmRSMrgIsMade = myAveragerIsMade = false;

    myNodeCount  = nodeCount;
    myEvalPCount = evalPointCount;
  }

  FFlFEElmResult(const FFlFEElmResult&) = delete;

  virtual ~FFlFEElmResult()
  {
    if (ENRSMrgs) delete[] ENRSMrgs;
    if (EPRSMrgs) delete[] EPRSMrgs;
  }

  FFlFEElmResult& operator=(const FFlFEElmResult&) = delete;

  // Management of temporary result merge operations for each element node
  // or evaluation point

  bool isENRSMergerMade(int nodeIdx) const
  {
    if (ENRSMrgs && nodeIdx < myNodeCount)
      return ENRSMrgs[nodeIdx] != NULL;

    return false;
  }

  bool isEPRSMergerMade(int nodeIdx) const
  {
    if (EPRSMrgs && nodeIdx < myNodeCount)
      return EPRSMrgs[nodeIdx] != NULL;

    return false;
  }

  FFaOperationBase* getENRSMerger(int nodeIdx) const
  {
    if (ENRSMrgs && nodeIdx < myNodeCount)
      if ((size_t)ENRSMrgs[nodeIdx] != 0x1)
        return ENRSMrgs[nodeIdx];

    return NULL;
  }

  FFaOperationBase* getEPRSMerger(int nodeIdx) const
  {
    if (EPRSMrgs && nodeIdx < myNodeCount)
      if ((size_t)EPRSMrgs[nodeIdx] != 0x1)
        return EPRSMrgs[nodeIdx];

    return NULL;
  }

  void setENRSMerger(int nodeIdx, FFaOperationBase* op)
  {
    if (nodeIdx >= myNodeCount) return;

    if (!ENRSMrgs) {
      ENRSMrgs = new FFaOperationBase*[myNodeCount];
      for (short int i = 0; i < myNodeCount; i++) ENRSMrgs[i] = NULL;
    }

    ENRSMrgs[nodeIdx] = op ? op : (FFaOperationBase*)0x1;
  }

  void setEPRSMerger(int nodeIdx, FFaOperationBase* op)
  {
    if (nodeIdx >= myEvalPCount) return;

    if (!EPRSMrgs) {
      EPRSMrgs = new FFaOperationBase*[myEvalPCount];
      for (short int i = 0; i < myEvalPCount; i++) EPRSMrgs[i] = NULL;
    }

    EPRSMrgs[nodeIdx] = op ? op : (FFaOperationBase*)0x1;
  }

  void setElmRSMerger(FFaOperationBase* op)
  {
    elmResSetMrg = op;
    myElmRSMrgIsMade = true;
  }

  // Temporary operation storage :

  FFaOperationBase* elmResSetMrg;
  FFaOperationBase* myAverager;

  char myNodeCount;
  char myEvalPCount;

  bool myElmRSMrgIsMade : 1;
  bool myAveragerIsMade : 1;

private:
  FFaOperationBase** ENRSMrgs;
  FFaOperationBase** EPRSMrgs;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};


/////////////////////////////////////////////////////////////////////////
//
// FFlFENodeResult
//
/////////////////////////////////////////////////////////////////////////

class FFlFENodeResult : public FFlFEResultBase
{
public:
  FFlFENodeResult()
  {
    resSetMerger = myAverager = NULL;
    myResSetMergerIsMade = myAveragerIsMade = false;
    myAveragerIsLineOnly = true;
  }

  virtual ~FFlFENodeResult() {}

  // Temporary operation storage :

  FFaOperationBase* resSetMerger;
  FFaOperationBase* myAverager;

  bool myResSetMergerIsMade : 1;
  bool myAveragerIsMade     : 1;
  bool myAveragerIsLineOnly : 1;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};

#endif
