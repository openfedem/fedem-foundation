// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaOpUtils.C
  \brief Utilities for accessing unary operations.
  \author Jacob Storen
*/

#include "FFaLib/FFaOperation/FFaOpUtils.H"


std::vector<std::string>
FFaOpUtils::findOpers(const std::string& varType, bool silence)
{
  std::vector<std::string> opers;
  if (varType.empty())
    return opers;
  else if (varType == "NUMBER")
    FFaUnaryOp<double,int>::getOperations(opers);
  else if (varType == "SCALAR")
    FFaUnaryOp<double,double>::getOperations(opers);
  else if (varType == "VECTOR")
    FFaUnaryOp<double,DoubleVec>::getOperations(opers);
  else if (varType == "VEC3")
    FFaUnaryOp<double,FaVec3>::getOperations(opers);
  else if (varType == "TMAT34")
    FFaUnaryOp<double,FaMat34>::getOperations(opers);
  else if (varType == "TMAT33")
    FFaUnaryOp<double,FaMat33>::getOperations(opers);
  else if (varType == "ROT3")
    FFaUnaryOp<double,FaVec3>::getOperations(opers);
  else if (varType == "TENSOR3")
    FFaUnaryOp<double,FFaTensor3>::getOperations(opers);
  else if (varType == "TENSOR2")
    FFaUnaryOp<double,FFaTensor2>::getOperations(opers);
  else if (varType == "TENSOR1")
    FFaUnaryOp<double,FFaTensor1>::getOperations(opers);
  else if (!silence)
    std::cerr <<" *** FFaOpUtils::findOpers: Unknown variable type \""
              << varType <<"\""<< std::endl;

  return opers;
}


std::string
FFaOpUtils::getDefaultOper(const std::string& varType, bool silence)
{
  if (varType.empty())
    return "";
  else if (varType == "NUMBER")
    return FFaUnaryOp<double,int>::getDefaultOperation();
  else if (varType == "SCALAR")
    return FFaUnaryOp<double,double>::getDefaultOperation();
  else if (varType == "VECTOR")
    return FFaUnaryOp<double,DoubleVec>::getDefaultOperation();
  else if (varType == "VEC3")
    return FFaUnaryOp<double,FaVec3>::getDefaultOperation();
  else if (varType == "TMAT34")
    return FFaUnaryOp<double,FaMat34>::getDefaultOperation();
  else if (varType == "TMAT33")
    return FFaUnaryOp<double,FaMat33>::getDefaultOperation();
  else if (varType == "ROT3")
    return FFaUnaryOp<double,FaVec3>::getDefaultOperation();
  else if (varType == "TENSOR3")
    return FFaUnaryOp<double,FFaTensor3>::getDefaultOperation();
  else if (varType == "TENSOR2")
    return FFaUnaryOp<double,FFaTensor2>::getDefaultOperation();
  else if (varType == "TENSOR1")
    return FFaUnaryOp<double,FFaTensor1>::getDefaultOperation();
  else if (!silence)
    std::cerr <<" *** FFaOpUtils::getDefaultOper: Unknown variable type \""
              << varType << std::endl;

  return "";
}
