// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrReadOpImpl.C
  \brief Implementation of read operations for different variable types.
*/

#include "FFrLib/FFrReadOp.H"
#include "FFrLib/FFrReadOpInit.H"
#include "FFrLib/FFrVariable.H"
#include "FFrLib/FFrVariableReference.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaAlgebra/FFaMat33.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaAlgebra/FFaTensor1.H"
#include "FFaLib/FFaAlgebra/FFaTensor2.H"
#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaPatterns/FFaGenericFactory.H"


namespace
{
  //! \brief Helper class for creating a read operation for a variable.
  class CreatorType
  {
  public:
    //! \brief The constructor initializes \ref dataType and \ref dataSize.
    CreatorType(const std::string& t, int s) : dataType(t), dataSize(s) {}

    //! \brief Global comparison operator, for sorting.
    friend bool operator<(const CreatorType& op1, const CreatorType& op2)
    {
      if (op1.dataType == op2.dataType)
        return op1.dataSize < op2.dataSize;
      else
        return op1.dataType < op2.dataType;
    }

#ifdef FFA_DEBUG
    //! \brief Global oputput stream operator.
    friend std::ostream& operator<< (std::ostream& os, const CreatorType& op)
    {
      return os << op.dataType <<":"<< op.dataSize;
    }
#endif

  private:
    std::string dataType; //!< Variable type name
    int         dataSize; //!< Size of the variable in bits
  };

  //! \brief Convenience type alias.
  using OperationFactory = FFaGenericFactory<FFaOperationBase,CreatorType,
                                             FFrVariableReference*>;

  //! Set to \e true when initialized, to avoid initializing more than once.
  bool initialized = false;

  float  bufS[12]; //!< Internal single precision buffer
  double bufD[12]; //!< Internal double orecision buffer

  using DoubleVec = std::vector<double>; //!< Convenience type alias
}


FFaOperationBase* FFrVariableReference::getReadOperation()
{
  return OperationFactory::instance()->create(CreatorType(myVariable->dataClass,
                                                          myVariable->dataSize),
                                              this);
}


//! \brief Convenience macro for registering a read operation creator.
#define REGISTER_CREATOR(Name,Size,Type) \
  OperationFactory::instance()->registerCreator(CreatorType(Name,Size), \
                                                FFaDynCB2S(FFrReadOp<Type>::create, \
                                                           FFrVariableReference*,FFaOperationBase*&))


void FFr::initReadOps()
{
  if (initialized) return;

  REGISTER_CREATOR( "NUMBER" , 32, int );

  REGISTER_CREATOR( "SCALAR" , 32, float );
  REGISTER_CREATOR( "SCALAR" , 64, double );
  REGISTER_CREATOR( "SCALAR" , 32, double );

  REGISTER_CREATOR( "VEC3"   , 64, FaVec3 );
  REGISTER_CREATOR( "VEC3"   , 32, FaVec3 ); // use FFaVec3f when ready

  REGISTER_CREATOR( "ROT3"   , 64, FaVec3 );
  REGISTER_CREATOR( "ROT3"   , 32, FaVec3 ); // as above - and the same for all float-types of AlgebraClasses

  REGISTER_CREATOR( "TMAT33" , 64, FaMat33 );
  REGISTER_CREATOR( "TMAT33" , 32, FaMat33 );

  REGISTER_CREATOR( "TMAT34" , 64, FaMat34 );
  REGISTER_CREATOR( "TMAT34" , 32, FaMat34 );

  REGISTER_CREATOR( "VECTOR" , 64, DoubleVec );
  REGISTER_CREATOR( "VECTOR" , 32, DoubleVec );

  REGISTER_CREATOR( "TENSOR1", 64, FFaTensor1 );
  REGISTER_CREATOR( "TENSOR1", 32, FFaTensor1 );

  REGISTER_CREATOR( "TENSOR2", 64, FFaTensor2 );
  REGISTER_CREATOR( "TENSOR2", 32, FFaTensor2 );

  REGISTER_CREATOR( "TENSOR3", 64, FFaTensor3 );
  REGISTER_CREATOR( "TENSOR3", 32, FFaTensor3 );

#undef REGISTER_CREATOR

  initialized = true;
}


void FFr::clearReadOps()
{
  OperationFactory::removeInstance();
  initialized = false;
}


template<class RetType>
bool FFrReadOp<RetType>::hasData() const
{
  return myRdbVar ? myRdbVar->hasDataForCurrentKey() : false;
}


template<class RetType>
bool FFrReadOp<RetType>::evaluate(RetType& value)
{
  return myRdbVar->readPositionedTimestepData(&value,1) > 0;
}


//! \brief Specialization for double.
template<> bool FFrReadOp<double>::evaluate(double& value)
{
  int nread = 0;
  switch (myRdbVar->getWordLength())
    {
    case 32:
      nread = myRdbVar->readPositionedTimestepData(bufS,1);
      value = static_cast<double>(bufS[0]);
      break;

    case 64:
      nread = myRdbVar->readPositionedTimestepData(&value,1);
      break;
    }

  return nread > 0;
}


//! \brief Specialization for std::vector<double>.
template<> bool FFrReadOp<DoubleVec>::evaluate(DoubleVec& value)
{
  int nread = 0;
  int nvals = myRdbVar->getNumWords();
  float* fv = bufS;
  switch (myRdbVar->getWordLength())
    {
    case 32:
      if (nvals > 12)
        fv = new float[nvals];
      nread = myRdbVar->readPositionedTimestepData(fv,nvals);
      value.resize(nvals,0.0);
      for (int i = 0; i < nvals && i < nread; i++)
        value[i] = static_cast<double>(fv[i]);
      if (nvals > 12)
        delete[] fv;
      break;

    case 64:
      value.resize(nvals,0.0);
      nread = myRdbVar->readPositionedTimestepData(value.data(),nvals);
      break;
    }

  return nread > 0;
}


//! \brief Specialization for FaMat33.
template<> bool FFrReadOp<FaMat33>::evaluate(FaMat33& value)
{
  int nread = 0;
  switch (myRdbVar->getWordLength())
    {
    case 32:
      nread = myRdbVar->readPositionedTimestepData(bufS,9);
      value = FaMat33(bufS);
      break;

    case 64:
      nread = myRdbVar->readPositionedTimestepData(bufD,9);
      value = FaMat33(bufD);
      break;
    }

  return nread > 0;
}


//! \brief Specialization for FaMat34.
template<> bool FFrReadOp<FaMat34>::evaluate(FaMat34& value)
{
  int nread = 0;
  switch (myRdbVar->getWordLength())
    {
    case 32:
      nread = myRdbVar->readPositionedTimestepData(bufS,12);
      value = FaMat34(bufS);
      break;

    case 64:
      nread = myRdbVar->readPositionedTimestepData(bufD,12);
      value = FaMat34(bufD);
      break;
    }

  return nread > 0;
}


//! \brief Specialization for FaVec3.
template<> bool FFrReadOp<FaVec3>::evaluate(FaVec3& value)
{
  int nread = 0;
  switch (myRdbVar->getWordLength())
    {
    case 32:
      nread = myRdbVar->readPositionedTimestepData(bufS,3);
      value = FaVec3(bufS);
      break;

    case 64:
      nread = myRdbVar->readPositionedTimestepData(bufD,3);
      value = FaVec3(bufD);
      break;
    }

  return nread > 0;
}


//! \brief Specialization for FFaTensor1.
template<> bool FFrReadOp<FFaTensor1>::evaluate(FFaTensor1& value)
{
  int nread = 0;
  switch (myRdbVar->getWordLength())
    {
    case 32:
      nread = myRdbVar->readPositionedTimestepData(bufS,1);
      value = FFaTensor1(bufS[0]);
      break;

    case 64:
      nread = myRdbVar->readPositionedTimestepData(bufD,1);
      value = FFaTensor1(bufD[0]);
      break;
    }

  return nread > 0;
}


//! \brief Specialization for FFaTensor2.
template<> bool FFrReadOp<FFaTensor2>::evaluate(FFaTensor2& value)
{
  int nread = 0;
  switch (myRdbVar->getWordLength())
    {
    case 32:
      nread = myRdbVar->readPositionedTimestepData(bufS,3);
      value = FFaTensor2(bufS);
      break;

    case 64:
      nread = myRdbVar->readPositionedTimestepData(bufD,3);
      value = FFaTensor2(bufD);
      break;
    }

  return nread > 0;
}


//! \brief Specialization for FFaTensor3.
template<> bool FFrReadOp<FFaTensor3>::evaluate(FFaTensor3& value)
{
  int nread = 0;
  switch (myRdbVar->getWordLength())
    {
    case 32:
      nread = myRdbVar->readPositionedTimestepData(bufS,6);
      value = FFaTensor3(bufS);
      break;

    case 64:
      nread = myRdbVar->readPositionedTimestepData(bufD,6);
      value = FFaTensor3(bufD);
      break;
    }

  return nread > 0;
}
