// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaOpUtils.H
  \brief Utilities for accessing unary operations.
  \author Jacob Storen
*/

#ifndef FFA_OP_UTILS_H
#define FFA_OP_UTILS_H

#include <string>

#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaAlgebra/FFaTensor2.H"
#include "FFaLib/FFaAlgebra/FFaTensor1.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaOperation/FFaOperation.H"


namespace
{
  //! \brief Tries to cast an operation to a given type and name.
  template <class ToType, class FromType>
  bool tryConverting(FFaOperation<ToType>*& to, FFaOperationBase* from,
                     const std::string& operationName, const FromType&)
  {
    FFaOperation<FromType>* fp = dynamic_cast<FFaOperation<FromType>*>(from);
    if (!fp) return false;

    to = new FFaUnaryOp<ToType,FromType>(fp,operationName);
    return true;
  }
}


namespace FFaOpUtils //! Utilities for accessing unary operations
{
  typedef std::vector<double> DoubleVec; //!< Convenience type definition

  //! \brief Converts an operation based on name and return type.
  template <class RetType>
  bool getUnaryConvertOp(FFaOperation<RetType>*& to,
                         FFaOperationBase* from,
                         const std::string& operationName)
  {
    if (from)
    {
      if (tryConverting(to,from,operationName,double())) return true;
      if (tryConverting(to,from,operationName,float())) return true;
      if (tryConverting(to,from,operationName,int())) return true;
      if (tryConverting(to,from,operationName,DoubleVec())) return true;
      if (tryConverting(to,from,operationName,FaVec3())) return true;
      if (tryConverting(to,from,operationName,FFaTensor3())) return true;
      if (tryConverting(to,from,operationName,FFaTensor2())) return true;
      if (tryConverting(to,from,operationName,FFaTensor1())) return true;
      if (tryConverting(to,from,operationName,FaMat34())) return true;
      if (tryConverting(to,from,operationName,FaMat33())) return true;
      from->unref(); // Invalid convertion, erase the object to avoid leakage
    }
    std::cerr <<" *** FFaOpUtils::getUnaryConvertOp: Invalid operation name \""
              << operationName <<"\""<< std::endl;
    to = NULL;
    return false;
  }

  //! \brief Returns valid operation names for a given variable type.
  std::vector<std::string> findOpers(const std::string& varType,
                                     bool silence = false);

  //! \brief Checks if given variable type has any operation defined.
  inline bool hasOpers(const std::string& varType)
  {
    return !findOpers(varType,true).empty();
  }

  //! \brief Returns default operation name for a given variable type.
  std::string getDefaultOper(const std::string& varType,
                             bool silence = false);
}

#endif
