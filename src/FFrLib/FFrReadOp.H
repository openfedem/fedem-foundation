// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_READ_OP_H
#define FFR_READ_OP_H

#ifdef FFA_DEBUG
#include <iostream>
#endif

#include "FFaLib/FFaOperation/FFaOperation.H"
#include "FFaLib/FFaPatterns/FFaGenericFactory.H"
#ifdef FT_USE_MEMPOOL
#include "FFaLib/FFaPatterns/FFaMemPool.H"
#endif

class FFrVariableReference;


class ReadOpCreatorType
{
public:
  ReadOpCreatorType(const std::string& t, int s = -1) : dataType(t), dataSize(s) {}

  friend bool operator< (const ReadOpCreatorType& op1, const ReadOpCreatorType& op2)
  {
    if (op1.dataType == op2.dataType)
      return op1.dataSize < op2.dataSize;
    else
      return op1.dataType < op2.dataType;
  }

#ifdef FFA_DEBUG
  friend std::ostream& operator<< (std::ostream& os, const ReadOpCreatorType& op)
  {
    return os << op.dataType <<":"<< op.dataSize;
  }
#endif

private:
  std::string dataType;
  int dataSize;
};


template <class RetType>
class FFrReadOp : public FFaOperation<RetType>
{
public:
  FFrReadOp(FFrVariableReference* vr) : myRdbVar(vr) {}

  static void create(FFrVariableReference* rdbVar, FFaOperationBase*& created)
  {
    created = new FFrReadOp<RetType>(rdbVar);
  }

  virtual bool hasData() const;
  virtual bool evaluate(RetType& value);

protected:
  virtual ~FFrReadOp() {}

private:
  FFrVariableReference* myRdbVar;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};

#ifdef FT_USE_MEMPOOL
//! \cond DO_NOT_DOCUMENT
template <class RetType>
FFaMemPool FFrReadOp<RetType>::ourMemPool = FFaMemPool(sizeof(FFrReadOp<RetType>),
						       FFaOperationBase::getMemPoolMgr());
//! \endcond
#endif


typedef FFaGenericFactory<FFaOperationBase,ReadOpCreatorType,FFrVariableReference*> OperationFactory;

#endif
