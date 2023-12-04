// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlIOAdaptors/FFlAllIOAdaptors.H"
#include "FFlLib/FFlIOAdaptors/FFlVTFWriter.H"
#include "FFlLib/FFlFEParts/FFlAllFEParts.H"
#include <iostream>
#include <cstring>
#include <cctype>


/*!
  \brief Simple FEM model to VTF conversion utility.

  This program reads a specified FEM model file in any of the supported format,
  and writes out the FE geometry to a VTF-file for visualization in GLview.
*/

int main (int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout <<"usage: "<< argv[0] <<" <femfile> [ASCII|BINARY|EXPRESS]\n";
    return 0;
  }

  int type = 2;
  if (argc > 2)
    switch (toupper(argv[2][0])) {
    case 'A': type = 0; break;
    case 'B': type = 1; break;
    }

  FFl::initAllReaders();
  FFl::initAllElements();

  FFlLinkHandler link;
  if (FFlReaders::instance()->read(argv[1],&link) > 0)
    std::cout <<"\n   * Read done, found "
              << link.buildFiniteElementVec() <<" elements."<< std::endl;
  else
    return 1;

  FFlVTFWriter vtf(&link);
  std::string partName(strtok(argv[1],"."));
  std::cout <<"   * Writing VTF-file "<< partName <<".vtf"<< std::endl;
  return vtf.write(partName+".vtf",partName,1,type) ? 0 : 2;
}
