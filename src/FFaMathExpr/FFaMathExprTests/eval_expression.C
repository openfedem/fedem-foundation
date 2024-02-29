// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMathExpr/FFaMathExprFactory.H"
#include <iostream>


int eval_expression (const char* mathExpr, const std::vector<double>& args, double& f)
{
  const int id = 1; // Uniqe ID of the expression

  if (args.empty())
    FFaMathExprFactory::instance()->create(id,mathExpr);
  else
    FFaMathExprFactory::instance()->create(id,mathExpr,args.size());

  int ierr = 0;
  if (args.size() == 1)
    f = FFaMathExprFactory::instance()->getValue(id,args.front(),ierr);
  else if (!args.empty())
    f = FFaMathExprFactory::instance()->getValue(id,args.data(),ierr);
  else
    f = 0.0;

  FFaMathExprFactory::removeInstance();
  if (ierr)
    std::cerr <<" *** Failed to evaluate \""<< mathExpr
              <<"\" ierr = "<< ierr << std::endl;
  else if (!args.empty())
  {
    std::cout <<"f("<< args.front();
    for (size_t j = 1; j < args.size(); j++) std::cout <<","<< args[j];
    std::cout <<") = "<< f << std::endl;
  }
  return ierr;
}
