// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FiDeviceFunctions/FiRAOTable.H"
#include <fstream>
#include <cstdlib>


int main (int argc, char** argv)
{
  std::cout <<"Hello, world."<< std::endl;

  if (argc < 3) return 1;
  std::ifstream inp(argv[1]);
  double phi = atof(argv[2]);
  std::cout << argv[1] <<" "<< phi << std::endl;

  FiRAOTable rao;
  if (rao.readDirection(inp,phi))
    std::cout <<"RAO table for direction "<< phi <<"\n"<< rao << std::endl;
  else
    std::cout <<"Failed to read RAO table"<< std::endl;

  return 0;
}
