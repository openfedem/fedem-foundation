// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#define FFL_INIT_ONLY
#include "FFlLib/FFlFEParts/FFlAllFEParts.H"
#undef FFL_INIT_ONLY
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlIOAdaptors/FFlVTFWriter.H"
#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cctype>

#ifdef FF_NAMESPACE
using namespace FF_NAMESPACE;
#endif


/*!
  \brief Simple geometry model to VTF conversion utility.

  This program reads a specified geometry file in any of the supported format,
  and writes out the FE geometry to a VTF-file for visualization in GLview.
*/

int main (int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout <<"usage: "<< argv[0]
              <<" <geofile> [ASCII|BINARY|EXPRESS] [<duplTol>]\n";
    return 0;
  }

  int type = 2;
  if (argc > 2)
    switch (toupper(argv[2][0])) {
    case 'A': type = 0; break;
    case 'B': type = 1; break;
    }

  std::ifstream cad(argv[1],std::ios::in);
  if (!cad) return 1;

  double dupTol = argc > 3 ? atof(argv[3]) : -1.0;
  FFaBody::prefix = FFaFilePath::getPath(argv[1]);
  FFaBody* body = FFaBody::readFromCAD(cad,dupTol);
  if (!body) return 2;

  std::cout <<"\n# Vertices: "<< body->getNoVertices()
            <<"\n# Faces   : "<< body->getNoFaces() << std::endl;

  FFlInit initializer;
  FFlLinkHandler link;
  for (size_t v = 0; v < body->getNoVertices(); v++)
    link.addNode(new FFlNode(v+1,body->getVertex(v)));
  for (size_t f = 0; f < body->getNoFaces(); f++)
  {
    int inod;
    std::vector<int> mnpc;
    for (int i = 0; (inod = body->getFaceVtx(f,i)) >= 0; i++)
      mnpc.push_back(1+inod);

    FFlElementBase* newElem = NULL;
    switch (mnpc.size()) {
    case 3: newElem = ElementFactory::instance()->create("TRI3",1+f); break;
    case 4: newElem = ElementFactory::instance()->create("QUAD4",1+f); break;
    default: std::cout <<"  ** Ignoring "<< mnpc.size() <<"-noded element\n";
    }
    if (newElem)
    {
      newElem->setNodes(mnpc);
      link.addElement(newElem);
    }
  }

  if (link.resolve() && link.getFiniteElement(1))
    link.dump();
  else
  {
    std::cerr <<" *** Failed to resolve FE model"<< std::endl;
    return 3;
  }

  FFlVTFWriter vtf(&link);
  std::string partName(strtok(argv[1],"."));
  std::cout <<"   * Writing VTF-file "<< partName <<".vtf"<< std::endl;
  return vtf.write(partName+".vtf",partName,1,type) ? 0 : 4;
}
