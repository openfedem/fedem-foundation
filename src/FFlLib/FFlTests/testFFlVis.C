// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlInit.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlVisualization/FFlGroupPartCreator.H"
#include <iostream>

#ifdef FF_NAMESPACE
using namespace FF_NAMESPACE;
#endif


/*!
  \brief Simple test program for the FFlVisualization module.
*/

int main (int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout <<"usage: "<< argv[0] <<" <femfile>\n";
    return 0;
  }

  FFlInit initializer;
  FFlLinkHandler link;
  if (int stat = FFlReaders::instance()->read(argv[1],&link); stat <= 0)
    return stat < 0 ? stat : 1;

  std::cout <<"\n   * Read done."<< std::endl;
  link.dump(argv[1]);

  FFlGroupPartCreator gp(&link);
  gp.makeLinkParts();
  std::cout <<"\n   * Making link parts done.\n"<< std::endl;
  gp.dump();
  return 0;
}
