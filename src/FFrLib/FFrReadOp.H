// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrReadOp.H
  \brief Operation for reading result variables from the results database.
*/

#ifndef FFR_READ_OP_H
#define FFR_READ_OP_H

#include "FFaLib/FFaOperation/FFaOperation.H"
#ifdef FT_USE_MEMPOOL
#include "FFaLib/FFaPatterns/FFaMemPool.H"
#endif

class FFrVariableReference;


/*!
  \brief Template class representing an operation for reading a result variable.
*/

template <class RetType>
class FFrReadOp : public FFaOperation<RetType>
{
protected:
  //! \brief The constructor initializes \ref myRdbVar.
  explicit FFrReadOp(FFrVariableReference* vr) : myRdbVar(vr) {}

public:
  //! \brief Static method for instantiating a read operation for \a rdbVar.
  static void create(FFrVariableReference* rdbVar, FFaOperationBase*& created)
  {
    created = new FFrReadOp<RetType>(rdbVar);
  }

  //! \brief Checks if this operation will retun any data,
  bool hasData() const override;
  //! \brief Invokes the actual read operation.
  bool evaluate(RetType& value) override;

private:
  FFrVariableReference* myRdbVar; //!< The variable to receive the data read

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

#endif
