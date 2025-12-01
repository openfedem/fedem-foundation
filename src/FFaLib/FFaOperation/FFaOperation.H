// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaOperation.H
  \brief Classes that make up the core of a operation/transformation system.

  \details An operation is a kind of function with other functions as arguments.
  Trees are built by setting operations as arguments to other operations.

  \author Jacob Storen
*/

#ifndef FFA_OPERATION_H
#define FFA_OPERATION_H

#include <string>
#include <vector>
#include <utility>

#include "FFaLib/FFaDynCalls/FFaDynCB.H"
#ifdef FT_USE_MEMPOOL
#include "FFaLib/FFaPatterns/FFaMemPool.H"
#endif


/*!
  \brief Base class for all operations.
  \details Operations are reference counting each other,
  and deleted when their reference count reaches zero.
*/

class FFaOperationBase
{
protected:
  //! \brief Default constructor.
  FFaOperationBase() { myRefCount = 0; }
  //! \brief Empty destructor.
  virtual ~FFaOperationBase() {}

public:
  //! \brief Increments the reference counter.
  void ref() { myRefCount++; }
  //! \brief Decrements the reference counter, and deletes *this if zero.
  void unref() { if (--myRefCount <= 0) delete this; }

#ifdef FT_USE_MEMPOOL
  //! \brief Frees the memory pool associated with the operation manager.
  static void freeMemPools() { getMemPoolMgr()->freeMemPools(); }
  //! \brief Returns a pointer to the operations memory pool manager.
  static FFaMemPoolMgr* getMemPoolMgr()
  {
    static FFaMemPoolMgr memPools;
    return &memPools;
  }
#else
  //! \brief Dummy method doing nothing.
  static void freeMemPools() {}
#endif

private:
  short int myRefCount; //!< Reference counter
};


/*!
  \brief The base class used as argument in other operations.
*/

template <class RetType>
class FFaOperation : public FFaOperationBase
{
protected:
  //! \brief Default constructor.
  FFaOperation() : IAmEvaluated(false) {}
  //! \brief Empty destructor.
  virtual ~FFaOperation() {}

public:
  //! \brief Returns current value of this operation.
  bool invoke(RetType& value)
  {
    bool ok = true;
    if (IAmEvaluated)
      value = myCachedValue;
    else
    {
      ok = this->evaluate(value);
      myCachedValue = value;
      IAmEvaluated = true;
    }

    return ok;
  }

  //! \brief Invalidates the cached value of this operation.
  virtual void invalidate() { IAmEvaluated = false; }

  //! \brief Checks if this operation will return any data.
  virtual bool hasData() const = 0;
  //! \brief Invokes the actual operation.
  virtual bool evaluate(RetType& value) = 0;

private:
  RetType myCachedValue; //!< Cached operation value, to avoid re-evaluation

protected:
  bool IAmEvaluated; //!< If \e true, the cached value will be used
};

//! \cond DO_NOT_DOCUMENT
template <> inline
FFaOperation<int>::FFaOperation() : myCachedValue(0), IAmEvaluated(false) {}

template <> inline
FFaOperation<float>::FFaOperation() : myCachedValue(0), IAmEvaluated(false) {}

template <> inline
FFaOperation<double>::FFaOperation() : myCachedValue(0), IAmEvaluated(false) {}
//! \endcond


/*!
  \brief A class used to manage and store callbacks of a certain type.
  \details This must be a base class of all operations that are supposed
  to have changeable calculations.
*/

template <class OperationCB>
class FFaOperationContainer
{
  //! \brief Convenience type alias for a named operations
  using NamedOper = std::pair<std::string,OperationCB>;

public:
  //! \brief Adds a named operation to the \a OperationCB container.
  static void addOperation(const std::string& name, OperationCB operation)
  {
    static std::vector<NamedOper> ops;
    ops.emplace_back(name,operation);
    ourOperations = &ops;
  }

  //! \brief Returns a list of all operations of type \a OperationCB.
  static void getOperations(std::vector<std::string>& names)
  {
    names.clear();
    if (ourOperations)
    {
      names.reserve(ourOperations->size());
      for (const NamedOper& oper : *ourOperations)
        names.push_back(oper.first);
    }
  }

  //! \brief Returns the default operation of type \a OperationCB.
  static const std::string& getDefaultOperation()
  {
    if (ourOperations && !ourOperations->empty())
      return ourOperations->front().first;

    static const std::string noop;
    return noop;
  }

protected:
  //! \brief The default constructor selects the default operation.
  FFaOperationContainer() { this->selectOperation(getDefaultOperation()); }

  //! \brief Selects the named operation to use.
  //! \details The given \a name has to be in the \ref ourOperations container.
  //! If not, this operation will be undefined and
  //! when invoked it will not change the output argument.
  void selectOperation(const std::string& name)
  {
    if (ourOperations && !name.empty())
      for (const NamedOper& oper : *ourOperations)
	if (oper.first == name)
        {
          myOperationCB = oper.second;
          return;
        }

    // The selected operation is not defined
    myOperationCB.erase();
  }

  OperationCB myOperationCB; //!< The selected operation

private:
  //! Static container for operations of type \a OperationCB.
  static std::vector<NamedOper>* ourOperations;
};

template <class OperationCB>
std::vector< std::pair<std::string,OperationCB> >*
FFaOperationContainer<OperationCB>::ourOperations = NULL;


/*!
  \brief A class for scalar operations.
*/

template <class RetType, class PrmType = RetType>
class FFaUnaryOp : public FFaOperation<RetType>,
                   public FFaOperationContainer< FFaDynCB2<RetType&,const PrmType&> >
{
public:
  //! \brief The constructor selects the actual operation to use from \a name.
  FFaUnaryOp(FFaOperation<PrmType>* param, const std::string& name = "")
  {
    if ((myParam = param))
      myParam->ref();

    if (!name.empty())
      this->selectOperation(name);
  }

  //! \brief Invalidates the cached value of this operation.
  virtual void invalidate()
  {
    if (this->IAmEvaluated && myParam)
      myParam->invalidate();

    this->IAmEvaluated = false;
  }

  //! \brief Checks if this operation will return any data.
  virtual bool hasData() const
  {
    return myParam ? myParam->hasData() : false;
  }

  //! \brief Invokes the actual operation.
  virtual bool evaluate(RetType& value)
  {
    bool ok = false;
    if (myParam && !this->myOperationCB.empty())
    {
      PrmType param;
      ok = myParam->invoke(param);
      this->myOperationCB.invoke(value,param);
    }

    return ok;
  }

protected:
  //! \brief The destructor deletes the child operation, if any.
  virtual ~FFaUnaryOp()
  {
    if (myParam) myParam->unref();
  }

private:
  FFaOperation<PrmType>* myParam; //!< Parameter child operation

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};

#ifdef FT_USE_MEMPOOL
//! \cond DO_NOT_DOCUMENT
template <class RetType, class PrmType>
FFaMemPool FFaUnaryOp<RetType,PrmType>::ourMemPool = FFaMemPool(sizeof(FFaUnaryOp<RetType,PrmType>),
                                                                FFaOperationBase::getMemPoolMgr());
//! \endcond
#endif


/*!
  \brief A class for vector operations.
*/

template <class RetType>
class FFaNToOneOp : public FFaOperation<RetType>,
                    public FFaOperationContainer< FFaDynCB2<RetType&,const std::vector<RetType>&> >
{
public:
  //! \brief The constructor selects the actual operation to use from \a name.
  FFaNToOneOp(const std::vector<FFaOperation<RetType>*>& params,
              const std::string& name = "")
  {
    myParams = params;
    for (FFaOperation<RetType>* p : myParams)
      if (p) p->ref();

    if (!name.empty())
      this->selectOperation(name);
  }

  //! \brief Invalidates the cached value of this operation.
  virtual void invalidate()
  {
    if (this->IAmEvaluated)
      for (FFaOperation<RetType>* p : myParams)
        if (p) p->invalidate();

    this->IAmEvaluated = false;
  }

  //! \brief Checks if this operation will return any data.
  virtual bool hasData() const
  {
    for (FFaOperation<RetType>* p : myParams)
      if (p && p->hasData())
        return true;

    return false;
  }

  //! \brief Invokes the actual operation.
  virtual bool evaluate(RetType& value)
  {
    bool ok = !this->myOperationCB.empty();
    if (ok)
    {
      std::vector<RetType> pVals(myParams.size());
      for (size_t i = 0; i < myParams.size(); i++)
        if (myParams[i])
          ok &= myParams[i]->invoke(pVals[i]);

      this->myOperationCB.invoke(value,pVals);
    }

    return ok;
  }

protected:
  //! \brief The destructor deletes the child operations, if any.
  virtual ~FFaNToOneOp()
  {
    for (FFaOperation<RetType>* p : myParams)
      if (p) p->unref();
  }

private:
  std::vector<FFaOperation<RetType>*> myParams; //!< Parameter child operations

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};

#ifdef FT_USE_MEMPOOL
//! \cond DO_NOT_DOCUMENT
template <class RetType>
FFaMemPool FFaNToOneOp<RetType>::ourMemPool = FFaMemPool(sizeof(FFaNToOneOp<RetType>),
                                                         FFaOperationBase::getMemPoolMgr());
//! \endcond
#endif


namespace FFa
{
  //! \brief Defines a special result value and its associated equivalent value.
  //! \details The special result value is used by some unary operations
  //! transforming result vectors into a double value. When \a value is
  //! encountered among the input values for an operation, it will be
  //! replaced by \a equiv before the actual operation is invoked.
  void setSpecialResultValue(double value, double equiv);
}

#endif
