// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <iostream>
#include <cstdlib>

int eval_expression (const char* mathExpr, const std::vector<double>& args, double& f);


int main (int argc, const char** argv)
{
  if (argc < 1)
  {
    std::cerr <<"usage: "<< argv[0] <<" (expression) [<x> [<y> [<z> [<t>]]]]\n";
    return 99;
  }

  std::vector<double> args;
  for (int i = 2; i < argc; i++)
    args.push_back(atof(argv[i]));

  double fVal;
  return eval_expression(argv[1],args,fVal);
}
