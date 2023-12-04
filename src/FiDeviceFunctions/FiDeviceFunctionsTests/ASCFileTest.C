// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FiDeviceFunctions/FiASCFile.H"
#include <iostream>
#include <cstdlib>


int main (int argc, char** argv)
{
  std::cout <<"Hello, world."<< std::endl;

  if (argc < 2) return 1;
  int nchan = FiASCFile::getNoChannels(argv[1]);
  std::cout <<"Number of channels in "<< argv[1] <<" is "<< nchan << std::endl;

  FiASCFile f(argv[1],nchan);
  if (!f.open())
  {
    std::cerr <<" *** Failed to open "<< argv[1] << std::endl;
    return 2;
  }

  double x = argc > 2 ? atof(argv[2]) : 0.0;
  int chan = argc > 3 ? atoi(argv[3]) : 1;

  std::cout <<"The file has "<< f.getValueCount() <<" data points.\n"
            <<"The value at x="<< x <<" for channel "<< chan <<" is "
            << f.getValue(x,chan,false,0.0,1.0) << std::endl;
  return 0;
}
