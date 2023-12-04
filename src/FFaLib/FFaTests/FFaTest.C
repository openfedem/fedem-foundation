// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>

#include "FFaLib/FFaCmdLineArg/FFaCmdLineArg.H"


int BodyTest (const std::string& fname, double z0, double z1);
int GeometryTest (const char* fname, std::istream* pointdata = NULL);


int main (int argc, char** argv)
{
  int ierr = 99;
  if (argc > 2 && strcmp(argv[1],"-body") == 0)
  {
    double z0 = argc > 3 ? atof(argv[3]) : 0.0;
    double z1 = argc > 4 ? atof(argv[4]) : z0;
    if ((ierr = BodyTest(argv[2],z0,z1)))
      std::cerr <<" *** BodyTest returned "<< ierr << std::endl;
  }
  else if (argc > 2 && strncmp(argv[1],"-geo",4) == 0)
  {
    if (argc > 3)
    {
      std::stringstream pointdata;
      for (int i = 3; i < argc; i++)
        pointdata <<" "<< argv[i];
      ierr = GeometryTest(argv[2],&pointdata);
    }
    else
      ierr = GeometryTest(argv[2]);
    if (ierr)
      std::cerr <<" *** GeometryTest returned "<< ierr << std::endl;
  }
  else if (argc > 1)
  {
    std::vector<double> v1({1.0,2.0}), v2;
    std::vector<int> v3({3,4,5}), v4;

    FFaCmdLineArg::init(argc,argv);
    FFaCmdLineArg::instance()->addOption("a","","String option");
    FFaCmdLineArg::instance()->addOption("b","jalla","String with default");
    FFaCmdLineArg::instance()->addOption("c",true,"Bool default on");
    FFaCmdLineArg::instance()->addOption("d",false,"Bool default off");
    FFaCmdLineArg::instance()->addOption("e",123,"Integer option");
    FFaCmdLineArg::instance()->addOption("f",456.0f,"Float option");
    FFaCmdLineArg::instance()->addOption("g",678.0,"Double option");
    FFaCmdLineArg::instance()->addOption("h1",v1,"Double vector option");
    FFaCmdLineArg::instance()->addOption("h2",v2,"Double vector (empty)");
    FFaCmdLineArg::instance()->addOption("i1",v3,"Integer vector option");
    FFaCmdLineArg::instance()->addOption("i2",v4,"Integer vector (empty)");

    std::string helpText;
    FFaCmdLineArg::instance()->composeHelpText(helpText);
    std::cout <<"Available command-line options:\n"<< helpText << std::endl;
    FFaCmdLineArg::instance()->listOptions();
    FFaCmdLineArg::instance()->getValue("h1",v2);
    FFaCmdLineArg::instance()->getValue("i1",v4);
    std::cout <<"\nh1:"; for (double v : v2) std::cout <<" "<< v;
    std::cout <<"\ni1:"; for (int intV : v4) std::cout <<" "<< intV;
    std::cout << std::endl;
  }
  else
    std::cerr <<"usage: "<< argv[0] <<" [-body <filename> [z0 [z1] |"
              <<" -geometry <filename> [points]]"
              << std::endl;

  return ierr;
}
