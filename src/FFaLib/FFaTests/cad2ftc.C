// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include <fstream>
#include <cstdlib>


/*!
  \brief Simple CAD model to FTC conversion utility.

  This program reads a specified CAD model file in any of the supported formats
  and writes out the geometry to FT's internal CAD format.
*/

int main (int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout <<"usage: "<< argv[0] <<" <cadfile> [<duplTol>]\n";
    return 0;
  }

  std::string fname(argv[1]);
  std::ifstream cad(fname,std::ios::in);
  if (!cad) return 2;

  double tol = argc > 2 ? atof(argv[2]) : -1.0;
  FFaBody::prefix = FFaFilePath::getPath(fname);
  FFaBody* body = FFaBody::readFromCAD(cad,tol);
  if (!body) return 3;

  std::cout <<"\n# Vertices: "<< body->getNoVertices()
            <<"\n# Faces   : "<< body->getNoFaces() << std::endl;
  cad.close();

  if (!body->writeCAD(FFaFilePath::addExtension(fname,"ftc"),FaMat34()))
    return 4;

  std::cout <<"Wrote "<< fname << std::endl;
  delete body;
  return 0;
}
