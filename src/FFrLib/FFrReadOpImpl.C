// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

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


//! Set to \e true when initialized, to avoid initializing more than once.
static bool initialized = false;


void FFr::initReadOps()
{
  if (initialized) return;

#define REGISTER_CREATOR(Name,Size,Type) \
  OperationFactory::instance()->registerCreator(ReadOpCreatorType(Name,Size), \
                                                FFaDynCB2S(FFrReadOp<Type>::create, \
                                                           FFrVariableReference*,FFaOperationBase*&))

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

  REGISTER_CREATOR( "VECTOR" , 64, std::vector<double> );
  REGISTER_CREATOR( "VECTOR" , 32, std::vector<double> );

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
bool FFrReadOp<RetType>::evaluate(RetType& value)
{
  return myRdbVar->readPositionedTimestepData(&value[0],sizeof(value)/sizeof(double)) > 0;
}


template<class RetType>
bool FFrReadOp<RetType>::hasData() const
{
  return myRdbVar ? myRdbVar->hasDataForCurrentKey() : false;
}


static float  bufS[12];
static double bufD[12];

template <>
bool FFrReadOp<double>::evaluate(double& value)
{
  int nread = 0;
  switch (myRdbVar->variableDescr->dataSize)
    {
    case 32:
      nread = myRdbVar->readPositionedTimestepData(bufS,1);
      value = bufS[0];
      break;

    case 64:
      nread = myRdbVar->readPositionedTimestepData(&value,1);
      break;
    }

  return nread > 0;
}


template<>
bool FFrReadOp<std::vector<double> >::evaluate(std::vector<double>& value)
{
  int nread = 0;
  int nvals = myRdbVar->variableDescr->getRepeats();
  switch (myRdbVar->variableDescr->dataSize)
    {
    case 32:
      {
	value.resize(nvals,0.0);
	std::vector<float> flVals(nvals,0.0f);
	nread = myRdbVar->readPositionedTimestepData(&flVals.front(),nvals);
	for (int i = 0; i < nvals; i++)
	  value[i] = flVals[i];
	break;
      }

    case 64:
      value.resize(nvals,0.0);
      nread = myRdbVar->readPositionedTimestepData(&value.front(),nvals);
      break;
    }

  return nread > 0;
}


template <>
bool FFrReadOp<FaMat33>::evaluate(FaMat33& value)
{
  int nread = 0;
  switch (myRdbVar->variableDescr->dataSize)
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


template<>
bool FFrReadOp<FaMat34>::evaluate(FaMat34& value)
{
  int nread = 0;
  switch (myRdbVar->variableDescr->dataSize)
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


template <>
bool FFrReadOp<FaVec3>::evaluate(FaVec3& value)
{
  int nread = 0;
  switch (myRdbVar->variableDescr->dataSize)
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


template<>
bool FFrReadOp<FFaTensor1>::evaluate(FFaTensor1& value)
{
  int nread = 0;
  switch (myRdbVar->variableDescr->dataSize)
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


template <>
bool FFrReadOp<FFaTensor2>::evaluate(FFaTensor2& value)
{
  int nread = 0;
  switch (myRdbVar->variableDescr->dataSize)
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


template<>
bool FFrReadOp<FFaTensor3>::evaluate(FFaTensor3& value)
{
  int nread = 0;
  switch (myRdbVar->variableDescr->dataSize)
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


template<>
bool FFrReadOp<float>::evaluate(float& value)
{
  return myRdbVar->readPositionedTimestepData(&value,1) > 0;
}


template<>
bool FFrReadOp<int>::evaluate(int& value)
{
  return myRdbVar->readPositionedTimestepData(&value,1) > 0;
}
